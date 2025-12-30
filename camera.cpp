#include "camera.h"
#include <QtMath>

QVector3D Camera::forward() const
{
    QVector3D dir;
    dir.setX(cos(qDegreesToRadians(yaw)) * cos(qDegreesToRadians(pitch)));
    dir.setY(sin(qDegreesToRadians(pitch)));
    dir.setZ(sin(qDegreesToRadians(yaw)) * cos(qDegreesToRadians(pitch)));
    return dir.normalized();
}

QVector3D Camera::right() const
{
    return QVector3D::crossProduct(forward(), {0, 1, 0}).normalized();
}

QMatrix4x4 Camera::viewMatrix() const
{
    QMatrix4x4 view;
    view.lookAt(position, position + forward(), {0, 1, 0});
    return view;
}
