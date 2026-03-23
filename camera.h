#pragma once
#include <QVector3D>
#include <QMatrix4x4>
#include <QtMath>

class Camera
{
public:
    QVector3D position {0.0f, 3.0f, 35.0f};
    float yaw   = -90.0f;  // degrees
    float pitch = -15.0f;  // degrees

    float speed       = 8.0f;
    float sensitivity = 0.15f;
    float fov         = 60.0f;

    QVector3D forward() const {
        float radYaw   = qDegreesToRadians(yaw);
        float radPitch = qDegreesToRadians(pitch);
        return QVector3D(
                   cos(radPitch)*cos(radYaw),
                   sin(radPitch),
                   cos(radPitch)*sin(radYaw)
                   ).normalized();
    }

    QVector3D right() const {
        return QVector3D::crossProduct(forward(), QVector3D(0,1,0)).normalized();
    }

    QVector3D up() const {
        return QVector3D::crossProduct(right(), forward()).normalized();
    }

    QMatrix4x4 viewMatrix() const {
        QMatrix4x4 view;
        view.lookAt(position, position + forward(), up());
        return view;
    }
};
