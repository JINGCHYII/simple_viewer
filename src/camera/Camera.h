#pragma once

#include <QMatrix4x4>
#include <QPoint>
#include <QSet>
#include <QVector3D>
#include <Qt>

class Camera
{
public:
    virtual ~Camera() = default;

    virtual QMatrix4x4 viewMatrix() const = 0;
    virtual QMatrix4x4 projMatrix(float aspectRatio) const = 0;

    virtual void handleMousePress(Qt::MouseButton button, const QPoint &pos) = 0;
    virtual void handleMouseMove(const QPoint &pos) = 0;
    virtual void handleMouseRelease(Qt::MouseButton button, const QPoint &pos) = 0;
    virtual void handleWheel(int delta) = 0;

    virtual void handleKeyPress(int key) = 0;
    virtual void handleKeyRelease(int key) = 0;
    virtual void update(float deltaSeconds) = 0;

    virtual QVector3D position() const = 0;
    virtual QVector3D forward() const = 0;
    virtual QVector3D up() const = 0;
};
