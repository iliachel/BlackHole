#pragma once

#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QString>
#include <QVector3D>
#include <QVector2D>

#include "camera.h"
#include "physics/geodesicintegrator.h"

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core
{
    Q_OBJECT

public:
    explicit GLWidget(QWidget* parent = nullptr);
    ~GLWidget();

    Camera camera;
    QElapsedTimer frameTimer;
    bool firstMouse = true;
    QPoint lastMousePos;
    bool keyW = false;
    bool keyA = false;
    bool keyS = false;
    bool keyD = false;
    bool keyQ = false;
    bool keyE = false;
    bool keyShift = false;
    bool mouseLookActive = false;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    void updatePhysicsDiagnostics();
    void updateCamera(float dt);
    void syncCameraFromOrbit();

    QOpenGLShaderProgram photonShader;
    QOpenGLShaderProgram gridShader;

    GLuint quadVao = 0;
    GLuint quadVbo = 0;
    GLuint gridVao = 0;
    GLuint gridVbo = 0;

    int gridIndexCount = 0;

    physics::KerrSpacetime spacetime {2.0, 0.9};
    physics::GeodesicIntegrator geodesicIntegrator {spacetime};
    QString physicsSummary;

    float orbitDistance = 20.0f;
    float orbitAzimuth = -90.0f;
    float orbitElevation = 0.0f;
};
