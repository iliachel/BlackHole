#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QElapsedTimer>
#include "camera.h"

// Структура должна быть выровнена для GLSL (std430 layout)
struct ParticleData {
    // x, y, z - координаты, w - не используется (padding) или масса
    float pos[4];
    // vx, vy, vz - компоненты 4-скорости, w - не используется
    float vel[4];
};

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget* parent = nullptr);
    ~GLWidget();
    // ... (код камеры и событий мыши остается тем же) ...

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void initParticles();

    QOpenGLShaderProgram renderShader;  // Шейдер для рисовки
    QOpenGLShaderProgram computeShader; // Шейдер для физики

    GLuint particleBuffer = 0; // SSBO ID
    GLuint vao = 0;

    int particleCount = 1000000; // Теперь мы можем потянуть МИЛЛИОН частиц

    // Параметры симуляции
    float blackHoleMass = 1.0f;
    float blackHoleSpin = 0.9f; // a parameter (0..1)
};
