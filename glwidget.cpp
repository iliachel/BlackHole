#include "glwidget.h"
#include <QMatrix4x4>
#include <cmath>

static constexpr int PARTICLE_COUNT = 100000;
static constexpr float G = 1.0f;
static constexpr float BLACK_HOLE_MASS = 50.0f;
static constexpr float EVENT_HORIZON = 0.5f;

void GLWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        mouseLookActive = true;
        lastMousePos = e->pos();
        setCursor(Qt::BlankCursor);   // hide cursor while rotating
    }
}


void GLWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!mouseLookActive)
        return;

    QPoint delta = e->pos() - lastMousePos;
    lastMousePos = e->pos();

    camera.yaw   += delta.x() * camera.sensitivity;
    camera.pitch -= delta.y() * camera.sensitivity;

    camera.pitch = qBound(-89.0f, camera.pitch, 89.0f);
}


void GLWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_W) keyW = false;
    if (e->key() == Qt::Key_S) keyS = false;
    if (e->key() == Qt::Key_A) keyA = false;
    if (e->key() == Qt::Key_D) keyD = false;
    if (e->key() == Qt::Key_Q) keyQ = false;
    if (e->key() == Qt::Key_E) keyE = false;
}


void GLWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_W) keyW = true;
    if (e->key() == Qt::Key_S) keyS = true;
    if (e->key() == Qt::Key_A) keyA = true;
    if (e->key() == Qt::Key_D) keyD = true;
    if (e->key() == Qt::Key_Q) keyQ = true;
    if (e->key() == Qt::Key_E) keyE = true;
}

void GLWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        mouseLookActive = false;
        unsetCursor();                // restore cursor
        firstMouse = true;
    }
}


GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    connect(&timer, &QTimer::timeout, this, QOverload<>::of(&GLWidget::update));
    timer.start(16);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    frameTimer.start();

}

GLWidget::~GLWidget()
{
    makeCurrent();
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    doneCurrent();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // ===== SHADERS =====
    shader.addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
    #version 430 core

    layout (location = 0) in vec3 pos;

    uniform mat4 mvp;
    out float vDist;

    void main()
    {
        vDist = length(pos);
        gl_Position = mvp * vec4(pos, 1.0);
        gl_PointSize = 2.0;
    }
)");

    shader.addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
    #version 430 core

    in float vDist;
    out vec4 FragColor;

    void main()
    {
        // Fade near event horizon
        float alpha = smoothstep(0.0, 1.0, vDist);

        float intensity = clamp(1.0 / vDist, 0.0, 1.0);
        vec3 color = mix(vec3(1.0, 0.4, 0.1),
                         vec3(0.8, 0.9, 1.0),
                         intensity);

        FragColor = vec4(color, alpha);
    }
)");

    shader.link();

    // ===== PARTICLES =====
    initParticles();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 particles.size() * sizeof(QVector3D),
                 nullptr,
                 GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
}

void GLWidget::initParticles()
{
    particles.clear();
    particles.reserve(PARTICLE_COUNT);

    for (int i = 0; i < PARTICLE_COUNT; ++i)
    {
        float r = 2.0f + static_cast<float>(rand()) / RAND_MAX * 8.0f;
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;

        QVector3D pos(
            r * cos(angle),
            ((rand() % 100) / 100.0f - 0.5f) * 0.1f,
            r * sin(angle)
            );

        float v = sqrt(G * BLACK_HOLE_MASS / r);
        QVector3D vel(
            -v * sin(angle),
            0.0f,
            v * cos(angle)
            );

        particles.push_back({pos, vel, 1.0f});
    }
}

void GLWidget::updatePhysics(float dt)
{
    for (auto& p : particles)
    {
        QVector3D r = p.pos;
        float d = r.length();

        // === EVENT HORIZON ===
        if (d < EVENT_HORIZON)
        {
            // Respawn particle
            float r0 = 6.0f + (rand() / float(RAND_MAX)) * 6.0f;
            float angle = (rand() / float(RAND_MAX)) * 2.0f * M_PI;

            p.pos = QVector3D(
                r0 * cos(angle),
                0.0f,
                r0 * sin(angle)
                );

            float v = sqrt(G * BLACK_HOLE_MASS / r0);
            p.vel = QVector3D(
                -v * sin(angle),
                0.0f,
                v * cos(angle)
                );

            p.life = 1.0f;
            continue;
        }

        QVector3D gravity =
            -G * BLACK_HOLE_MASS / (d * d) * r.normalized();

        p.vel += gravity * dt;
        p.pos += p.vel * dt;
    }
}


void GLWidget::paintGL()
{

    float dt = frameTimer.restart() / 1000.0f;

    if (keyW) camera.position += camera.forward() * camera.speed * dt;
    if (keyS) camera.position -= camera.forward() * camera.speed * dt;
    if (keyA) camera.position -= camera.right()   * camera.speed * dt;
    if (keyD) camera.position += camera.right()   * camera.speed * dt;
    if (keyQ) camera.position.setY(camera.position.y() - camera.speed * dt);
    if (keyE) camera.position.setY(camera.position.y() + camera.speed * dt);

    updatePhysics(timeStep);

    std::vector<QVector3D> positions;
    positions.reserve(particles.size());
    for (const auto& p : particles)
        positions.push_back(p.pos);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    positions.size() * sizeof(QVector3D),
                    positions.data());

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 proj;
    proj.perspective(camera.fov,
                     float(width()) / height(),
                     0.1f,
                     500.0f);

    QMatrix4x4 view = camera.viewMatrix();
    QMatrix4x4 model;

    shader.bind();
    shader.setUniformValue("mvp", proj * view * model);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, particles.size());

    shader.release();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
