#pragma once

#include "camera/Camera.h"

class OrbitCamera : public Camera
{
public:
    OrbitCamera();

    QMatrix4x4 viewMatrix() const override;
    QMatrix4x4 projMatrix(float aspectRatio) const override;

    void handleMousePress(Qt::MouseButton button, const QPoint &pos) override;
    void handleMouseMove(const QPoint &pos) override;
    void handleMouseRelease(Qt::MouseButton button, const QPoint &pos) override;
    void handleWheel(int delta) override;

    void handleKeyPress(int key) override;
    void handleKeyRelease(int key) override;
    void update(float deltaSeconds) override;

    QVector3D position() const override;
    QVector3D forward() const override;
    QVector3D up() const override;

    void setFromView(const QVector3D &position, const QVector3D &forward, const QVector3D &up);

private:
    QVector3D right() const;

    QVector3D m_target{0.0f, 0.0f, 0.0f};
    float m_distance{5.0f};
    float m_yaw{-90.0f};
    float m_pitch{20.0f};

    QPoint m_lastMousePos;
    bool m_rotating{false};
    bool m_panning{false};
};
