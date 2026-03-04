#include "camera/FlyCamera.h"

#include <QKeyEvent>
#include <QtMath>

namespace
{
constexpr float kFovDegrees = 60.0f;
constexpr float kNear = 0.05f;
constexpr float kFar = 3000.0f;
constexpr float kLookSensitivity = 0.18f;
constexpr float kPitchLimit = 89.0f;
constexpr float kBoostMultiplier = 3.0f;
}

FlyCamera::FlyCamera() = default;

QMatrix4x4 FlyCamera::viewMatrix() const
{
    QMatrix4x4 view;
    view.lookAt(m_position, m_position + m_front, m_up);
    return view;
}

QMatrix4x4 FlyCamera::projMatrix(float aspectRatio) const
{
    QMatrix4x4 proj;
    proj.perspective(kFovDegrees, aspectRatio, kNear, kFar);
    return proj;
}

void FlyCamera::handleMousePress(Qt::MouseButton button, const QPoint &pos)
{
    if (button == Qt::LeftButton) {
        m_rotating = true;
        m_lastMousePos = pos;
    }
}

void FlyCamera::handleMouseMove(const QPoint &pos)
{
    if (!m_rotating) {
        return;
    }

    const QPoint delta = pos - m_lastMousePos;
    m_lastMousePos = pos;

    m_yaw += delta.x() * kLookSensitivity;
    m_pitch -= delta.y() * kLookSensitivity;
    m_pitch = qBound(-kPitchLimit, m_pitch, kPitchLimit);

    QVector3D front;
    const float yawRad = qDegreesToRadians(m_yaw);
    const float pitchRad = qDegreesToRadians(m_pitch);

    front.setX(qCos(yawRad) * qCos(pitchRad));
    front.setY(qSin(pitchRad));
    front.setZ(qSin(yawRad) * qCos(pitchRad));
    m_front = front.normalized();

    const QVector3D worldUp(0.0f, 1.0f, 0.0f);
    m_up = QVector3D::crossProduct(right(), m_front).normalized();
    if (qFuzzyIsNull(m_up.lengthSquared())) {
        m_up = worldUp;
    }
}

void FlyCamera::handleMouseRelease(Qt::MouseButton button, const QPoint &)
{
    if (button == Qt::LeftButton) {
        m_rotating = false;
    }
}

void FlyCamera::handleWheel(int)
{
}

void FlyCamera::handleKeyPress(int key)
{
    m_pressedKeys.insert(key);
}

void FlyCamera::handleKeyRelease(int key)
{
    m_pressedKeys.remove(key);
}

void FlyCamera::update(float deltaSeconds)
{
    float velocity = m_speed * deltaSeconds;
    if (m_pressedKeys.contains(Qt::Key_Shift)) {
        velocity *= kBoostMultiplier;
    }

    if (m_pressedKeys.contains(Qt::Key_W)) {
        m_position += m_front * velocity;
    }
    if (m_pressedKeys.contains(Qt::Key_S)) {
        m_position -= m_front * velocity;
    }
    if (m_pressedKeys.contains(Qt::Key_A)) {
        m_position -= right() * velocity;
    }
    if (m_pressedKeys.contains(Qt::Key_D)) {
        m_position += right() * velocity;
    }
    if (m_pressedKeys.contains(Qt::Key_Q)) {
        m_position -= m_up * velocity;
    }
    if (m_pressedKeys.contains(Qt::Key_E)) {
        m_position += m_up * velocity;
    }
}

QVector3D FlyCamera::position() const
{
    return m_position;
}

QVector3D FlyCamera::forward() const
{
    return m_front.normalized();
}

QVector3D FlyCamera::up() const
{
    return m_up.normalized();
}

void FlyCamera::setFromView(const QVector3D &position, const QVector3D &forward, const QVector3D &up)
{
    m_position = position;
    m_front = forward.normalized();

    m_yaw = qRadiansToDegrees(qAtan2(m_front.z(), m_front.x()));
    m_pitch = qRadiansToDegrees(qAsin(m_front.y()));
    m_pitch = qBound(-kPitchLimit, m_pitch, kPitchLimit);

    const QVector3D rightVec = QVector3D::crossProduct(m_front, up).normalized();
    m_up = QVector3D::crossProduct(rightVec, m_front).normalized();
    if (qFuzzyIsNull(m_up.lengthSquared())) {
        m_up = QVector3D(0.0f, 1.0f, 0.0f);
    }
}

QVector3D FlyCamera::right() const
{
    const QVector3D worldUp(0.0f, 1.0f, 0.0f);
    const QVector3D rightVec = QVector3D::crossProduct(m_front, worldUp).normalized();
    if (qFuzzyIsNull(rightVec.lengthSquared())) {
        return QVector3D(1.0f, 0.0f, 0.0f);
    }
    return rightVec;
}
