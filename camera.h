#pragma once
#include <QVector3D>
#include <QMatrix4x4>

class Camera
{
public:
    QVector3D position {0.0f, 3.0f, 10.0f};
    float yaw   = -90.0f;
    float pitch = -15.0f;

    float speed       = 8.0f;
    float sensitivity = 0.15f;
    float fov         = 60.0f;

    QMatrix4x4 viewMatrix() const;
    QVector3D forward() const;
    QVector3D right() const;
};
