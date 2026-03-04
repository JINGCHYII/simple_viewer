#include "render/GLViewport.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

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
    m_pointCloud.clear();
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

    emit cameraModeChanged(m_cameraMode);
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
    emit renderModeChanged(mode);
    update();
}

GLViewport::RenderMode GLViewport::renderMode() const
{
    return m_renderMode;
}

void GLViewport::setPointSize(float size)
{
    m_pointSize = qBound(1.0f, size, 20.0f);
    update();
}

float GLViewport::pointSize() const
{
    return m_pointSize;
}

void GLViewport::setPointColorMode(PointColorMode mode)
{
    m_pointColorMode = mode;
    update();
}

GLViewport::PointColorMode GLViewport::pointColorMode() const
{
    return m_pointColorMode;
}

int GLViewport::vertexCount() const
{
    return m_vertexCount;
}

int GLViewport::faceCount() const
{
    return m_faceCount;
}

void GLViewport::frameAll()
{
    const QVector3D offset(1.0f, 0.6f, 1.0f);
    const QVector3D viewDir = offset.normalized();

    const float orbitDistance = qMax(0.2f, m_bboxRadius / qSin(qDegreesToRadians(22.5f)));
    m_orbitCamera.setTargetAndDistance(m_bboxCenter, orbitDistance, viewDir, QVector3D(0.0f, 1.0f, 0.0f));

    const float flyDistance = qMax(0.2f, m_bboxRadius / qSin(qDegreesToRadians(30.0f)));
    m_flyCamera.setFromView(m_bboxCenter - viewDir * flyDistance, viewDir, QVector3D(0.0f, 1.0f, 0.0f));
    update();
}

void GLViewport::resetCamera()
{
    m_orbitCamera = OrbitCamera();
    m_flyCamera = FlyCamera();
    frameAll();
}

void GLViewport::setFrontView()
{
    m_orbitCamera.setTargetAndDistance(m_bboxCenter, m_bboxRadius * 2.0f, QVector3D(0.0f, 0.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f));
    m_flyCamera.setFromView(m_bboxCenter + QVector3D(0.0f, 0.0f, m_bboxRadius * 2.0f), QVector3D(0.0f, 0.0f, -1.0f), QVector3D(0.0f, 1.0f, 0.0f));
    update();
}

void GLViewport::setRightView()
{
    m_orbitCamera.setTargetAndDistance(m_bboxCenter, m_bboxRadius * 2.0f, QVector3D(-1.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f));
    m_flyCamera.setFromView(m_bboxCenter + QVector3D(m_bboxRadius * 2.0f, 0.0f, 0.0f), QVector3D(-1.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f));
    update();
}

void GLViewport::setTopView()
{
    m_orbitCamera.setTargetAndDistance(m_bboxCenter, m_bboxRadius * 2.0f, QVector3D(0.0f, -1.0f, 0.0f), QVector3D(0.0f, 0.0f, -1.0f));
    m_flyCamera.setFromView(m_bboxCenter + QVector3D(0.0f, m_bboxRadius * 2.0f, 0.0f), QVector3D(0.0f, -1.0f, 0.0f), QVector3D(0.0f, 0.0f, -1.0f));
    update();
}

bool GLViewport::loadModel(const QString &path)
{
    ModelLoader loader;
    if (!loader.load(path)) {
        return false;
    }

    const bool hasMesh = loader.hasMesh();
    PointCloudData pointData = loader.pointCloudData();
    if (pointData.points.empty() && hasMesh) {
        pointData.points = loader.meshData().vertices;
    }
    const bool hasPointCloud = !pointData.points.empty();

    makeCurrent();
    if (hasMesh) {
        m_mesh.upload(loader.meshData());
    } else {
        m_mesh.clear();
    }
    if (hasPointCloud) {
        m_pointCloud.upload(pointData);
    } else {
        m_pointCloud.clear();
    }
    doneCurrent();

    const auto &vertices = hasMesh ? loader.meshData().vertices : pointData.points;
    if (!vertices.empty()) {
        m_bboxMin = QVector3D(vertices.front().pos.x, vertices.front().pos.y, vertices.front().pos.z);
        m_bboxMax = m_bboxMin;
        for (const Vertex &vertex : vertices) {
            const QVector3D point(vertex.pos.x, vertex.pos.y, vertex.pos.z);
            m_bboxMin.setX(qMin(m_bboxMin.x(), point.x()));
            m_bboxMin.setY(qMin(m_bboxMin.y(), point.y()));
            m_bboxMin.setZ(qMin(m_bboxMin.z(), point.z()));
            m_bboxMax.setX(qMax(m_bboxMax.x(), point.x()));
            m_bboxMax.setY(qMax(m_bboxMax.y(), point.y()));
            m_bboxMax.setZ(qMax(m_bboxMax.z(), point.z()));
        }
        m_bboxCenter = (m_bboxMin + m_bboxMax) * 0.5f;
        m_bboxRadius = qMax(0.001f, (m_bboxMax - m_bboxCenter).length());
    }

    m_vertexCount = static_cast<int>(vertices.size());
    m_faceCount = hasMesh ? static_cast<int>(loader.meshData().indices.size() / 3) : 0;

    if (!hasMesh) {
        setRenderMode(RenderMode::PointCloud);
    }

    frameAll();
    emit modelStatsChanged(m_vertexCount, m_faceCount);
    update();
    return hasMesh ? m_mesh.isValid() : m_pointCloud.isValid();
}

void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
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

    const bool drawMesh = m_renderMode != RenderMode::PointCloud && m_mesh.isValid();
    const bool drawPoints = (m_renderMode == RenderMode::PointCloud && m_pointCloud.isValid());
    if (!m_shader.isValid() || (!drawMesh && !drawPoints)) {
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
    m_shader.setInt(this, "uIsPointRender", drawPoints ? 1 : 0);
    m_shader.setFloat(this, "uPointSize", m_pointSize);
    m_shader.setInt(this, "uPointColorMode", m_pointColorMode == PointColorMode::VertexColor ? 0 : 1);
    m_shader.setVec3(this, "uBoundsMin", m_bboxMin);
    m_shader.setVec3(this, "uBoundsMax", m_bboxMax);

    if (drawPoints) {
        m_shader.setInt(this, "uEnableSpecular", 0);
        m_shader.setFloat(this, "uAmbientStrength", 0.65f);
        m_shader.setFloat(this, "uSpecularStrength", 0.0f);
        m_pointCloud.draw(this);
    } else if (m_renderMode == RenderMode::Wireframe) {
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
    switch (event->key()) {
    case Qt::Key_F:
        frameAll();
        event->accept();
        return;
    case Qt::Key_R:
        resetCamera();
        event->accept();
        return;
    case Qt::Key_1:
        setFrontView();
        event->accept();
        return;
    case Qt::Key_3:
        setRightView();
        event->accept();
        return;
    case Qt::Key_7:
        setTopView();
        event->accept();
        return;
    default:
        break;
    }

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

    PointCloudData pointData;
    pointData.points = meshData.vertices;
    m_pointCloud.upload(pointData);

    m_bboxMin = QVector3D(-1.0f, -1.0f, -1.0f);
    m_bboxMax = QVector3D(1.0f, 1.0f, 1.0f);
    m_bboxCenter = QVector3D(0.0f, 0.0f, 0.0f);
    m_bboxRadius = 1.732f;
    m_vertexCount = static_cast<int>(meshData.vertices.size());
    m_faceCount = static_cast<int>(meshData.indices.size() / 3);
}
