#include "render/GLViewport.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include "io/ModelData.h"
#include "io/ModelLoader.h"

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

GLViewport::~GLViewport()
{
    makeCurrent();
    m_mesh.clear();
    m_shader.clear(this);
    doneCurrent();
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

void GLViewport::setRenderMode(RenderMode mode)
{
    if (m_renderMode == mode) {
        return;
    }
    m_renderMode = mode;
    update();
}

GLViewport::RenderMode GLViewport::renderMode() const
{
    return m_renderMode;
}

bool GLViewport::loadModel(const QString &path)
{
    ModelLoader loader;
    if (!loader.load(path) || !loader.hasMesh()) {
        return false;
    }

    makeCurrent();
    m_mesh.upload(loader.meshData());
    doneCurrent();
    update();
    return m_mesh.isValid();
}

void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);

    m_shader.load(this, ":/shaders/shaders/basic.vert", ":/shaders/shaders/blinn_phong.frag");
    uploadDefaultMesh();
}

void GLViewport::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLViewport::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_shader.isValid() || !m_mesh.isValid()) {
        return;
    }

    const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    const QMatrix4x4 view = currentCamera()->viewMatrix();
    const QMatrix4x4 proj = currentCamera()->projMatrix(aspect);

    QMatrix4x4 model;

    const QVector3D lightPos(3.0f, 3.5f, 3.0f);
    const QVector3D viewPos = currentCamera()->position();

    m_shader.bind(this);
    m_shader.setMat4(this, "uModel", model);
    m_shader.setMat4(this, "uView", view);
    m_shader.setMat4(this, "uProj", proj);
    m_shader.setVec3(this, "uLightPos", lightPos);
    m_shader.setVec3(this, "uViewPos", viewPos);

    m_shader.setVec3(this, "uAmbientColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shader.setVec3(this, "uLightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shader.setVec3(this, "uMaterialColor", QVector3D(0.72f, 0.74f, 0.78f));
    m_shader.setFloat(this, "uAmbientStrength", 0.22f);
    m_shader.setFloat(this, "uSpecularStrength", 0.45f);
    m_shader.setFloat(this, "uShininess", 48.0f);
    m_shader.setInt(this, "uUseVertexColor", 1);
    m_shader.setInt(this, "uEnableSpecular", m_renderMode == RenderMode::Wireframe ? 0 : 1);

    if (m_renderMode == RenderMode::Wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        m_mesh.draw(this);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else if (m_renderMode == RenderMode::SolidWireOverlay) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        m_mesh.draw(this);

        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        m_shader.setInt(this, "uUseVertexColor", 0);
        m_shader.setVec3(this, "uMaterialColor", QVector3D(0.02f, 0.02f, 0.02f));
        m_shader.setFloat(this, "uAmbientStrength", 1.0f);
        m_shader.setFloat(this, "uSpecularStrength", 0.0f);
        m_shader.setInt(this, "uEnableSpecular", 0);
        m_mesh.draw(this);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        m_mesh.draw(this);
    }

    m_shader.release(this);
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

void GLViewport::uploadDefaultMesh()
{
    MeshData meshData;
    meshData.vertices = {
        {{-1.0f, -1.0f, -1.0f}, {-0.577f, -0.577f, -0.577f}, {0.83f, 0.28f, 0.28f}},
        {{1.0f, -1.0f, -1.0f}, {0.577f, -0.577f, -0.577f}, {0.28f, 0.83f, 0.28f}},
        {{1.0f, 1.0f, -1.0f}, {0.577f, 0.577f, -0.577f}, {0.28f, 0.28f, 0.83f}},
        {{-1.0f, 1.0f, -1.0f}, {-0.577f, 0.577f, -0.577f}, {0.83f, 0.83f, 0.28f}},
        {{-1.0f, -1.0f, 1.0f}, {-0.577f, -0.577f, 0.577f}, {0.28f, 0.83f, 0.83f}},
        {{1.0f, -1.0f, 1.0f}, {0.577f, -0.577f, 0.577f}, {0.83f, 0.28f, 0.83f}},
        {{1.0f, 1.0f, 1.0f}, {0.577f, 0.577f, 0.577f}, {0.88f, 0.88f, 0.88f}},
        {{-1.0f, 1.0f, 1.0f}, {-0.577f, 0.577f, 0.577f}, {0.48f, 0.48f, 0.48f}},
    };

    meshData.indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 4, 7, 7, 3, 0,
        1, 5, 6, 6, 2, 1,
        3, 2, 6, 6, 7, 3,
        0, 1, 5, 5, 4, 0,
    };

    m_mesh.upload(meshData);
}
