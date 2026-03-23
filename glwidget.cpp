#include "glwidget.h"

#include <QDebug>
#include <QWidget>

#include <algorithm>
#include <cmath>

namespace {

QMatrix3x3 makeCameraBasis(const Camera& cam)
{
    const QVector3D forward = cam.forward().normalized();
    const QVector3D right = QVector3D::crossProduct(forward, QVector3D(0.0f, 1.0f, 0.0f)).normalized();
    const QVector3D up = QVector3D::crossProduct(right, forward);

    QMatrix3x3 basis;
    basis(0, 0) = right.x();
    basis(0, 1) = right.y();
    basis(0, 2) = right.z();
    basis(1, 0) = up.x();
    basis(1, 1) = up.y();
    basis(1, 2) = up.z();
    basis(2, 0) = -forward.x();
    basis(2, 1) = -forward.y();
    basis(2, 2) = -forward.z();
    return basis;
}

} // namespace

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    orbitDistance = 20.0f;
    orbitAzimuth = -90.0f;
    orbitElevation = 0.0f;
    camera.speed = 10.0f;
    camera.sensitivity = 0.12f;
    syncCameraFromOrbit();

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    frameTimer.start();
}

GLWidget::~GLWidget()
{
    makeCurrent();
    if (quadVbo) {
        glDeleteBuffers(1, &quadVbo);
    }
    if (quadVao) {
        glDeleteVertexArrays(1, &quadVao);
    }
    doneCurrent();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    updatePhysicsDiagnostics();

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    photonShader.addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
#version 430 core
layout(location = 0) in vec2 pos;
out vec2 vUV;
void main() {
    vUV = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)");

    photonShader.addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
#version 430 core
out vec4 FragColor;

uniform vec3 camPos;
uniform mat3 camBasis;
uniform float M;
uniform float a;
uniform float fov;
uniform float aspect;
uniform vec2 resolution;

const int STEPS = 2200;
const float STEP = 0.025;
const float DISK_INNER = 5.2;
const float DISK_OUTER = 30.0;
const float DISK_HALF_THICKNESS = 0.22;

float horizonRadius()
{
    return M + sqrt(max(0.0, M * M - a * a));
}

vec3 getRay(vec2 uv)
{
    uv.x *= aspect;
    float z = -1.0 / tan(radians(fov) * 0.5);
    return normalize(camBasis * vec3(uv, z));
}

vec3 kerrAcceleration(vec3 pos)
{
    float r2 = dot(pos, pos);
    float r = sqrt(r2);
    if (r < 1e-3) {
        return vec3(0.0);
    }
    return -M * pos / (r2 * r);
}

float hash13(vec3 p)
{
    return fract(sin(dot(p, vec3(127.1, 311.7, 758.545))) * 43758.5453);
}

float starField(vec3 d)
{
    vec3 p = normalize(d) * 120.0;
    float coarse = hash13(floor(p));
    float fine = hash13(floor(p * 5.0));
    float bright = smoothstep(0.992, 0.9997, coarse);
    float sparkle = smoothstep(0.996, 0.9999, fine);
    return bright + sparkle * 0.65;
}

vec3 backgroundColor(vec3 dir)
{
    float vertical = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 deep = vec3(0.004, 0.006, 0.014);
    vec3 haze = vec3(0.016, 0.024, 0.055);
    vec3 bg = mix(deep, haze, vertical);
    float stars = starField(dir);
    vec3 warmStars = vec3(1.0, 0.96, 0.9) * stars;
    vec3 coldStars = vec3(0.72, 0.82, 1.0) * stars * 0.45;
    return bg + warmStars + coldStars;
}

vec3 diskEmission(vec3 pos, vec3 vel)
{
    float cylindricalR = length(pos.xz);
    if (cylindricalR < DISK_INNER || cylindricalR > DISK_OUTER || abs(pos.y) > DISK_HALF_THICKNESS) {
        return vec3(0.0);
    }

    float radial = clamp((cylindricalR - DISK_INNER) / (DISK_OUTER - DISK_INNER), 0.0, 1.0);
    float core = exp(-3.4 * radial);
    float rim = exp(-10.0 * abs(radial - 0.18));
    float thicknessFade = exp(-10.0 * abs(pos.y));

    vec3 tangent = normalize(vec3(-pos.z, 0.0, pos.x));
    float orbitalSpeed = sqrt(max(M / max(cylindricalR, 0.1), 0.0));
    float doppler = dot(tangent, -normalize(vel)) * orbitalSpeed * 1.8;

    vec3 hot = vec3(1.55, 0.92, 0.58);
    vec3 warm = vec3(1.0, 0.38, 0.12);
    vec3 color = mix(warm, hot, core);
    color *= 0.28 + 1.75 * core + 0.55 * rim;
    color *= thicknessFade;
    color *= 1.0 + doppler;
    return max(color, vec3(0.0));
}

vec3 horizonGlow(vec3 pos)
{
    float r = length(pos);
    float h = horizonRadius();
    float glow = exp(-2.2 * max(r - h, 0.0));
    float ring = exp(-7.5 * abs(r - (2.6 * h)));
    return vec3(1.0, 0.48, 0.12) * glow * 0.08 +
           vec3(1.0, 0.72, 0.32) * ring * 0.03;
}

void main()
{
    vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;

    vec3 pos = camPos;
    vec3 vel = getRay(uv);
    vec3 accumulated = vec3(0.0);

    float rH = horizonRadius();

    for (int i = 0; i < STEPS; ++i) {
        float r = length(pos);
        if (r < rH) {
            FragColor = vec4(accumulated, 1.0);
            return;
        }

        vec3 acc = kerrAcceleration(pos);
        accumulated += diskEmission(pos, vel) * 0.018;
        accumulated += horizonGlow(pos) * 0.01;
        vel += acc * STEP;
        vel = normalize(vel);
        pos += vel * STEP;
    }

    vec3 bg = backgroundColor(normalize(vel));
    vec3 color = bg + accumulated;
    color = vec3(1.0) - exp(-color);
    FragColor = vec4(color, 1.0);
}
)");

    if (!photonShader.link()) {
        qDebug() << "Photon shader link error:" << photonShader.log();
    }

    constexpr float quad[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
    glGenVertexArrays(1, &quadVao);
    glBindVertexArray(quadVao);

    glGenBuffers(1, &quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
}

void GLWidget::updatePhysicsDiagnostics()
{
    physics::BoyerLindquistState photon;
    photon.r = 18.0;
    photon.theta = 1.5707963267948966;
    photon.pt = -1.0;
    photon.pr = -0.2;
    photon.ptheta = 0.0;
    photon.pphi = 6.0;

    const auto samples = geodesicIntegrator.integrate(photon, 256, 0.01);
    if (samples.empty()) {
        return;
    }

    const double initialHamiltonian = samples.front().hamiltonian;
    const double finalHamiltonian = samples.back().hamiltonian;
    const double drift = std::abs(finalHamiltonian - initialHamiltonian);
    const double horizon = spacetime.horizonRadius();

    physicsSummary = QString("Kerr M=%1 a=%2 r+=%3 H0=%4 dH=%5")
                         .arg(spacetime.mass(), 0, 'f', 2)
                         .arg(spacetime.spin(), 0, 'f', 2)
                         .arg(horizon, 0, 'f', 3)
                         .arg(initialHamiltonian, 0, 'e', 3)
                         .arg(drift, 0, 'e', 3);

    qDebug().noquote() << physicsSummary;

    if (QWidget* topLevel = window()) {
        topLevel->setWindowTitle(QString("BlackHole Kerr Geodesics Lab | %1").arg(physicsSummary));
    }
}

void GLWidget::updateCamera(float dt)
{
    float speed = camera.speed;
    if (keyShift) {
        speed *= 3.0f;
    }

    if (keyW) {
        orbitDistance -= speed * dt;
    }
    if (keyS) {
        orbitDistance += speed * dt;
    }
    if (keyA) {
        orbitAzimuth -= speed * 5.0f * dt;
    }
    if (keyD) {
        orbitAzimuth += speed * 5.0f * dt;
    }
    if (keyQ) {
        orbitElevation += speed * 3.0f * dt;
    }
    if (keyE) {
        orbitElevation -= speed * 3.0f * dt;
    }

    orbitDistance = std::clamp(orbitDistance, 4.0f, 120.0f);
    orbitElevation = std::clamp(orbitElevation, -85.0f, 85.0f);
    syncCameraFromOrbit();
}

void GLWidget::syncCameraFromOrbit()
{
    camera.yaw = orbitAzimuth + 180.0f;
    camera.pitch = -orbitElevation;

    const float azimuthRad = qDegreesToRadians(orbitAzimuth);
    const float elevationRad = qDegreesToRadians(orbitElevation);
    const float cosElevation = std::cos(elevationRad);

    camera.position = QVector3D(
        orbitDistance * cosElevation * std::cos(azimuthRad),
        orbitDistance * std::sin(elevationRad),
        orbitDistance * cosElevation * std::sin(azimuthRad));
}

void GLWidget::paintGL()
{
    float dt = 0.016f;
    if (frameTimer.isValid()) {
        dt = std::clamp(frameTimer.restart() / 1000.0f, 0.001f, 0.033f);
    }
    const float safeHeight = static_cast<float>(std::max(1, height()));
    const float aspectRatio = static_cast<float>(width()) / safeHeight;
    updateCamera(dt);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const QMatrix3x3 camBasis = makeCameraBasis(camera);

    photonShader.bind();
    photonShader.setUniformValue("camPos", camera.position);
    photonShader.setUniformValue("camBasis", camBasis);
    photonShader.setUniformValue("M", static_cast<float>(spacetime.mass()));
    photonShader.setUniformValue("a", static_cast<float>(spacetime.spin()));
    photonShader.setUniformValue("fov", camera.fov);
    photonShader.setUniformValue("aspect", aspectRatio);
    photonShader.setUniformValue("resolution", QVector2D(width(), height()));

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    update();
}

void GLWidget::mousePressEvent(QMouseEvent* e)
{
    setFocus();
    if (e->button() == Qt::RightButton) {
        mouseLookActive = true;
        lastMousePos = e->pos();
        setCursor(Qt::BlankCursor);
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!mouseLookActive) {
        return;
    }

    const QPoint delta = e->pos() - lastMousePos;
    lastMousePos = e->pos();
    orbitAzimuth += delta.x() * camera.sensitivity;
    orbitElevation = qBound(-85.0f, orbitElevation - delta.y() * camera.sensitivity, 85.0f);
    syncCameraFromOrbit();
}

void GLWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton) {
        mouseLookActive = false;
        unsetCursor();
    }
}

void GLWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_W) {
        keyW = true;
    }
    if (e->key() == Qt::Key_S) {
        keyS = true;
    }
    if (e->key() == Qt::Key_A) {
        keyA = true;
    }
    if (e->key() == Qt::Key_D) {
        keyD = true;
    }
    if (e->key() == Qt::Key_Q) {
        keyQ = true;
    }
    if (e->key() == Qt::Key_E) {
        keyE = true;
    }
    if (e->key() == Qt::Key_Shift) {
        keyShift = true;
    }
}

void GLWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_W) {
        keyW = false;
    }
    if (e->key() == Qt::Key_S) {
        keyS = false;
    }
    if (e->key() == Qt::Key_A) {
        keyA = false;
    }
    if (e->key() == Qt::Key_D) {
        keyD = false;
    }
    if (e->key() == Qt::Key_Q) {
        keyQ = false;
    }
    if (e->key() == Qt::Key_E) {
        keyE = false;
    }
    if (e->key() == Qt::Key_Shift) {
        keyShift = false;
    }
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
