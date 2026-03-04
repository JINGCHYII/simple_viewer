#include "render/GLViewport.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

GLViewport::GLViewport(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    m_frameTimer.start();
    m_updateTimer.setInterval(16);
    connect(&m_updateTimer, &QTimer::timeout, this, [this]() {
        const qint64 elapsedMs = m_frameTimer.restart();
        const float deltaSeconds = static_cast<float>(elapsedMs) / 1000.0f;
        currentCamera()->update(deltaSeconds);
        update();
    });
    m_updateTimer.start();
}

void GLViewport::setCameraMode(CameraMode mode)
{
    if (m_cameraMode == mode) {
        return;
    }

    const QVector3D pos = currentCamera()->position();
    const QVector3D fwd = currentCamera()->forward();
    const QVector3D up = currentCamera()->up();

    m_cameraMode = mode;
    if (m_cameraMode == CameraMode::Orbit) {
        m_orbitCamera.setFromView(pos, fwd, up);
    } else {
        m_flyCamera.setFromView(pos, fwd, up);
    }

    update();
}

GLViewport::CameraMode GLViewport::cameraMode() const
{
    return m_cameraMode;
}

void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
}

void GLViewport::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLViewport::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera matrices are computed here so rendering pipeline can consume them.
    const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    const QMatrix4x4 view = currentCamera()->viewMatrix();
    const QMatrix4x4 proj = currentCamera()->projMatrix(aspect);

    Q_UNUSED(view);
    Q_UNUSED(proj);
}

void GLViewport::keyPressEvent(QKeyEvent *event)
{
    currentCamera()->handleKeyPress(event->key());
    event->accept();
}

void GLViewport::keyReleaseEvent(QKeyEvent *event)
{
    currentCamera()->handleKeyRelease(event->key());
    event->accept();
}

void GLViewport::wheelEvent(QWheelEvent *event)
{
    currentCamera()->handleWheel(event->angleDelta().y());
    event->accept();
    update();
}

void GLViewport::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    currentCamera()->handleMousePress(event->button(), event->pos());
    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    currentCamera()->handleMouseMove(event->pos());
    event->accept();
    update();
}

void GLViewport::mouseReleaseEvent(QMouseEvent *event)
{
    currentCamera()->handleMouseRelease(event->button(), event->pos());
    event->accept();
}

Camera *GLViewport::currentCamera()
{
    return m_cameraMode == CameraMode::Orbit ? static_cast<Camera *>(&m_orbitCamera)
                                              : static_cast<Camera *>(&m_flyCamera);
}

const Camera *GLViewport::currentCamera() const
{
    return m_cameraMode == CameraMode::Orbit ? static_cast<const Camera *>(&m_orbitCamera)
                                              : static_cast<const Camera *>(&m_flyCamera);
}
