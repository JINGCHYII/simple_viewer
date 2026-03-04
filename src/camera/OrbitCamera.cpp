#include "camera/OrbitCamera.h"

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
constexpr float kPitchLimit = 89.0f;
}

OrbitCamera::OrbitCamera() = default;

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
        m_yaw += delta.x() * kRotateSensitivity;
        m_pitch -= delta.y() * kRotateSensitivity;
        m_pitch = qBound(-kPitchLimit, m_pitch, kPitchLimit);
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
    const float yawRad = qDegreesToRadians(m_yaw);
    const float pitchRad = qDegreesToRadians(m_pitch);

    QVector3D offset;
    offset.setX(qCos(yawRad) * qCos(pitchRad));
    offset.setY(qSin(pitchRad));
    offset.setZ(qSin(yawRad) * qCos(pitchRad));

    return m_target - offset.normalized() * m_distance;
}

QVector3D OrbitCamera::forward() const
{
    return (m_target - position()).normalized();
}

QVector3D OrbitCamera::up() const
{
    return QVector3D::crossProduct(right(), forward()).normalized();
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
    const QVector3D worldUp(0.0f, 1.0f, 0.0f);
    return QVector3D::crossProduct(forward(), worldUp).normalized();
}

void OrbitCamera::setTargetAndDistance(const QVector3D &target, float distance, const QVector3D &forward, const QVector3D &)
{
    m_target = target;
    m_distance = qBound(kMinDistance, distance, kMaxDistance);

    const QVector3D dir = forward.normalized();
    m_yaw = qRadiansToDegrees(qAtan2(dir.z(), dir.x()));
    m_pitch = qRadiansToDegrees(qAsin(dir.y()));
    m_pitch = qBound(-kPitchLimit, m_pitch, kPitchLimit);
}
