#include "camera/OrbitCamera.h"

#include <cmath>
#include <QtMath>

namespace
{
constexpr float kFovDegrees = 45.0f;
constexpr float kNear = 0.1f;
constexpr float kFar = 2000.0f;
constexpr float kMinDistance = 0.2f;
constexpr float kMaxDistance = 200.0f;
constexpr float kRotateSensitivity = 0.25f;
constexpr float kPanSensitivity = 0.004f;
constexpr float kZoomSensitivity = 0.0009f;
}

OrbitCamera::OrbitCamera()
    : m_orientation(QQuaternion::fromDirection(QVector3D(0.0f, 0.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f)))
{
}

QMatrix4x4 OrbitCamera::viewMatrix() const
{
    QMatrix4x4 view;
    view.lookAt(position(), m_target, up());
    return view;
}

QMatrix4x4 OrbitCamera::projMatrix(float aspectRatio) const
{
    QMatrix4x4 proj;
    proj.perspective(kFovDegrees, aspectRatio, kNear, kFar);
    return proj;
}

void OrbitCamera::handleMousePress(Qt::MouseButton button, const QPoint &pos)
{
    m_lastMousePos = pos;
    if (button == Qt::LeftButton) {
        m_rotating = true;
    } else if (button == Qt::MiddleButton) {
        m_panning = true;
    }
}

void OrbitCamera::handleMouseMove(const QPoint &pos)
{
    const QPoint delta = pos - m_lastMousePos;
    m_lastMousePos = pos;

    if (m_rotating) {
        const QQuaternion yawRot = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), -delta.x() * kRotateSensitivity);
        const QQuaternion pitchRot = QQuaternion::fromAxisAndAngle(right(), delta.y() * kRotateSensitivity);
        m_orientation = (pitchRot * yawRot * m_orientation).normalized();
    }

    if (m_panning) {
        const float panScale = m_distance * kPanSensitivity;
        m_target -= right() * (delta.x() * panScale);
        m_target += up() * (delta.y() * panScale);
    }
}

void OrbitCamera::handleMouseRelease(Qt::MouseButton button, const QPoint &)
{
    if (button == Qt::LeftButton) {
        m_rotating = false;
    } else if (button == Qt::MiddleButton) {
        m_panning = false;
    }
}

void OrbitCamera::handleWheel(int delta)
{
    m_distance *= (1.0f - delta * kZoomSensitivity);
    m_distance = qBound(kMinDistance, m_distance, kMaxDistance);
}

void OrbitCamera::handleKeyPress(int)
{
}

void OrbitCamera::handleKeyRelease(int)
{
}

void OrbitCamera::update(float)
{
}

QVector3D OrbitCamera::position() const
{
    return m_target - forward() * m_distance;
}

QVector3D OrbitCamera::forward() const
{
    const QVector3D dir = m_orientation.rotatedVector(QVector3D(0.0f, 0.0f, -1.0f)).normalized();
    if (qFuzzyIsNull(dir.lengthSquared())) {
        return QVector3D(0.0f, 0.0f, -1.0f);
    }
    return dir;
}

QVector3D OrbitCamera::up() const
{
    const QVector3D upVec = m_orientation.rotatedVector(QVector3D(0.0f, 1.0f, 0.0f)).normalized();
    if (qFuzzyIsNull(upVec.lengthSquared())) {
        return QVector3D(0.0f, 1.0f, 0.0f);
    }
    return upVec;
}

void OrbitCamera::setFromView(const QVector3D &pos, const QVector3D &fwd, const QVector3D &up)
{
    setTargetAndDistance(pos + fwd.normalized() * qBound(kMinDistance, (m_target - pos).length(), kMaxDistance),
                         qBound(kMinDistance, (m_target - pos).length(), kMaxDistance),
                         fwd,
                         up);
}

QVector3D OrbitCamera::right() const
{
    const QVector3D rightVec = m_orientation.rotatedVector(QVector3D(1.0f, 0.0f, 0.0f)).normalized();
    if (qFuzzyIsNull(rightVec.lengthSquared())) {
        return QVector3D(1.0f, 0.0f, 0.0f);
    }
    return rightVec;
}

void OrbitCamera::setTargetAndDistance(const QVector3D &target, float distance, const QVector3D &forward, const QVector3D &)
{
    m_target = target;
    m_distance = qBound(kMinDistance, distance, kMaxDistance);
    const QVector3D dir = forward.normalized();
    if (qFuzzyIsNull(dir.lengthSquared())) {
        m_orientation = QQuaternion::fromDirection(QVector3D(0.0f, 0.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f));
    } else {
        m_orientation = QQuaternion::fromDirection(dir, QVector3D(0.0f, 1.0f, 0.0f));
    }
}
