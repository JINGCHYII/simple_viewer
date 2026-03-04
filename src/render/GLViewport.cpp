#include "render/GLViewport.h"

#include <algorithm>

#include <QFileInfo>
#include <QKeyEvent>
#include <QMatrix4x4>
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
    for (SceneModel &model : m_models) {
        model.mesh.clear();
        model.pointCloud.clear();
    }
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

void GLViewport::setAutoFitEnabled(bool enabled)
{
    m_autoFitEnabled = enabled;
}

bool GLViewport::autoFitEnabled() const
{
    return m_autoFitEnabled;
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
        emit logMessage(tr("导入失败: %1").arg(path), true);
        return false;
    }

    const bool hasMesh = loader.hasMesh();
    PointCloudData pointData = loader.pointCloudData();
    if (pointData.points.empty() && hasMesh) {
        pointData.points = loader.meshData().vertices;
    }
    const bool hasPointCloud = !pointData.points.empty();
    const auto &vertices = hasMesh ? loader.meshData().vertices : pointData.points;

    if (vertices.empty()) {
        emit logMessage(tr("导入失败(无可用顶点): %1").arg(path), true);
        return false;
    }

    SceneModel model;
    model.id = m_nextModelId++;
    model.path = path;
    model.name = QFileInfo(path).completeBaseName();
    if (model.name.isEmpty()) {
        model.name = QFileInfo(path).fileName();
    }
    model.hasMesh = hasMesh;
    model.hasPointCloud = hasPointCloud;

    makeCurrent();
    if (hasMesh) {
        model.mesh.upload(loader.meshData());
    }
    if (hasPointCloud) {
        model.pointCloud.upload(pointData);
    }
    doneCurrent();

    model.bboxMin = QVector3D(vertices.front().pos.x, vertices.front().pos.y, vertices.front().pos.z);
    model.bboxMax = model.bboxMin;
    for (const Vertex &vertex : vertices) {
        const QVector3D point(vertex.pos.x, vertex.pos.y, vertex.pos.z);
        model.bboxMin.setX(qMin(model.bboxMin.x(), point.x()));
        model.bboxMin.setY(qMin(model.bboxMin.y(), point.y()));
        model.bboxMin.setZ(qMin(model.bboxMin.z(), point.z()));
        model.bboxMax.setX(qMax(model.bboxMax.x(), point.x()));
        model.bboxMax.setY(qMax(model.bboxMax.y(), point.y()));
        model.bboxMax.setZ(qMax(model.bboxMax.z(), point.z()));
    }

    model.vertexCount = static_cast<int>(vertices.size());
    model.faceCount = hasMesh ? static_cast<int>(loader.meshData().indices.size() / 3) : 0;

    if (!hasMesh) {
        setRenderMode(RenderMode::PointCloud);
    }

    m_models.push_back(std::move(model));
    if (m_selectedModelId < 0) {
        m_selectedModelId = m_models.back().id;
        emit selectedModelChanged(m_selectedModelId);
    }

    recomputeSceneBounds();
    updateSceneStats();
    frameAll();
    emit modelListChanged();
    emit logMessage(tr("已导入模型: %1").arg(path), false);
    update();
    return true;
}

QVector<GLViewport::ModelInfo> GLViewport::modelInfos() const
{
    QVector<ModelInfo> infos;
    infos.reserve(m_models.size());
    for (const SceneModel &model : m_models) {
        infos.push_back({model.id,
                         model.name,
                         model.path,
                         model.visible,
                         model.vertexCount,
                         model.faceCount,
                         model.translation,
                         model.rotationEuler,
                         model.scale});
    }
    return infos;
}

bool GLViewport::setModelVisible(int modelId, bool visible)
{
    SceneModel *model = findModel(modelId);
    if (model == nullptr) {
        return false;
    }
    model->visible = visible;
    recomputeSceneBounds();
    if (m_autoFitEnabled) {
        frameAll();
    }
    update();
    return true;
}

bool GLViewport::removeModel(int modelId)
{
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models[i].id != modelId) {
            continue;
        }
        makeCurrent();
        m_models[i].mesh.clear();
        m_models[i].pointCloud.clear();
        doneCurrent();

        const QString removedName = m_models[i].name;
        m_models.removeAt(i);
        if (m_selectedModelId == modelId) {
            m_selectedModelId = m_models.isEmpty() ? -1 : m_models.front().id;
            emit selectedModelChanged(m_selectedModelId);
        }
        recomputeSceneBounds();
        updateSceneStats();
        const bool hasVisibleModels = std::any_of(m_models.cbegin(), m_models.cend(), [](const SceneModel &model) {
            return model.visible;
        });
        if (m_autoFitEnabled && hasVisibleModels) {
            frameAll();
        }
        emit modelListChanged();
        emit logMessage(tr("已删除模型: %1").arg(removedName), false);
        update();
        return true;
    }
    return false;
}

bool GLViewport::selectModel(int modelId)
{
    if (m_selectedModelId == modelId) {
        return true;
    }
    if (modelId >= 0 && findModel(modelId) == nullptr) {
        return false;
    }
    m_selectedModelId = modelId;
    emit selectedModelChanged(modelId);
    update();
    return true;
}

int GLViewport::selectedModelId() const
{
    return m_selectedModelId;
}

bool GLViewport::setModelTransform(int modelId, const QVector3D &translation, const QVector3D &rotationEuler, const QVector3D &scale)
{
    SceneModel *model = findModel(modelId);
    if (model == nullptr) {
        return false;
    }

    model->translation = translation;
    model->rotationEuler = rotationEuler;
    model->scale = QVector3D(qMax(0.001f, scale.x()), qMax(0.001f, scale.y()), qMax(0.001f, scale.z()));
    recomputeSceneBounds();
    if (m_autoFitEnabled) {
        frameAll();
    }
    update();
    return true;
}

void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glClearColor(0.05f, 0.08f, 0.12f, 1.0f);

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

    if (!m_shader.isValid() || m_models.isEmpty()) {
        return;
    }

    const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    const QMatrix4x4 view = currentCamera()->viewMatrix();
    const QMatrix4x4 proj = currentCamera()->projMatrix(aspect);

    const QVector3D lightPos(3.0f, 3.5f, 3.0f);
    const QVector3D viewPos = currentCamera()->position();

    m_shader.bind(this);
    m_shader.setMat4(this, "uView", view);
    m_shader.setMat4(this, "uProj", proj);
    m_shader.setVec3(this, "uLightPos", lightPos);
    m_shader.setVec3(this, "uViewPos", viewPos);

    m_shader.setVec3(this, "uAmbientColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shader.setVec3(this, "uLightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_shader.setFloat(this, "uAmbientStrength", 0.22f);
    m_shader.setFloat(this, "uSpecularStrength", 0.45f);
    m_shader.setFloat(this, "uShininess", 48.0f);
    m_shader.setInt(this, "uUseVertexColor", 1);
    m_shader.setInt(this, "uEnableSpecular", m_renderMode == RenderMode::Wireframe ? 0 : 1);
    m_shader.setFloat(this, "uPointSize", m_pointSize);
    m_shader.setInt(this, "uPointColorMode", m_pointColorMode == PointColorMode::VertexColor ? 0 : 1);

    for (const SceneModel &model : m_models) {
        if (!model.visible) {
            continue;
        }

        const bool drawPoints = (m_renderMode == RenderMode::PointCloud && model.hasPointCloud && model.pointCloud.isValid());
        const bool drawMesh = (m_renderMode != RenderMode::PointCloud && model.hasMesh && model.mesh.isValid());
        if (!drawPoints && !drawMesh) {
            continue;
        }

        m_shader.setMat4(this, "uModel", modelMatrix(model));
        m_shader.setInt(this, "uIsPointRender", drawPoints ? 1 : 0);
        m_shader.setVec3(this, "uBoundsMin", model.bboxMin);
        m_shader.setVec3(this, "uBoundsMax", model.bboxMax);

        const bool selected = model.id == m_selectedModelId;
        m_shader.setVec3(this, "uMaterialColor", selected ? QVector3D(0.87f, 0.83f, 0.58f) : QVector3D(0.72f, 0.74f, 0.78f));

        if (drawPoints) {
            m_shader.setInt(this, "uEnableSpecular", 0);
            m_shader.setFloat(this, "uAmbientStrength", selected ? 0.78f : 0.65f);
            m_shader.setFloat(this, "uSpecularStrength", 0.0f);
            model.pointCloud.draw(this);
            m_shader.setFloat(this, "uAmbientStrength", 0.22f);
            m_shader.setFloat(this, "uSpecularStrength", 0.45f);
        } else if (m_renderMode == RenderMode::Wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            model.mesh.draw(this);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else if (m_renderMode == RenderMode::SolidWireOverlay) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            model.mesh.draw(this);

            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0f, -1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            m_shader.setInt(this, "uUseVertexColor", 0);
            m_shader.setVec3(this, "uMaterialColor", QVector3D(0.02f, 0.02f, 0.02f));
            m_shader.setFloat(this, "uAmbientStrength", 1.0f);
            m_shader.setFloat(this, "uSpecularStrength", 0.0f);
            m_shader.setInt(this, "uEnableSpecular", 0);
            model.mesh.draw(this);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);

            m_shader.setInt(this, "uUseVertexColor", 1);
            m_shader.setFloat(this, "uAmbientStrength", 0.22f);
            m_shader.setFloat(this, "uSpecularStrength", 0.45f);
            m_shader.setInt(this, "uEnableSpecular", 1);
        } else {
            model.mesh.draw(this);
        }
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

    PointCloudData pointData;
    pointData.points = meshData.vertices;

    SceneModel model;
    model.id = m_nextModelId++;
    model.name = tr("默认立方体");
    model.path = tr("(builtin)");
    model.hasMesh = true;
    model.hasPointCloud = true;
    model.bboxMin = QVector3D(-1.0f, -1.0f, -1.0f);
    model.bboxMax = QVector3D(1.0f, 1.0f, 1.0f);
    model.vertexCount = static_cast<int>(meshData.vertices.size());
    model.faceCount = static_cast<int>(meshData.indices.size() / 3);

    model.mesh.upload(meshData);
    model.pointCloud.upload(pointData);

    m_models.push_back(std::move(model));
    m_selectedModelId = m_models.front().id;

    recomputeSceneBounds();
    updateSceneStats();
    emit modelListChanged();
    emit selectedModelChanged(m_selectedModelId);
}

GLViewport::SceneModel *GLViewport::findModel(int modelId)
{
    for (SceneModel &model : m_models) {
        if (model.id == modelId) {
            return &model;
        }
    }
    return nullptr;
}

const GLViewport::SceneModel *GLViewport::findModel(int modelId) const
{
    for (const SceneModel &model : m_models) {
        if (model.id == modelId) {
            return &model;
        }
    }
    return nullptr;
}

void GLViewport::recomputeSceneBounds()
{
    bool initialized = false;
    QVector3D sceneMin;
    QVector3D sceneMax;

    for (const SceneModel &model : m_models) {
        if (!model.visible) {
            continue;
        }
        const QMatrix4x4 mtx = modelMatrix(model);
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    const QVector3D local(x ? model.bboxMax.x() : model.bboxMin.x(),
                                          y ? model.bboxMax.y() : model.bboxMin.y(),
                                          z ? model.bboxMax.z() : model.bboxMin.z());
                    const QVector3D world = (mtx * QVector4D(local, 1.0f)).toVector3D();
                    if (!initialized) {
                        sceneMin = world;
                        sceneMax = world;
                        initialized = true;
                    } else {
                        sceneMin.setX(qMin(sceneMin.x(), world.x()));
                        sceneMin.setY(qMin(sceneMin.y(), world.y()));
                        sceneMin.setZ(qMin(sceneMin.z(), world.z()));
                        sceneMax.setX(qMax(sceneMax.x(), world.x()));
                        sceneMax.setY(qMax(sceneMax.y(), world.y()));
                        sceneMax.setZ(qMax(sceneMax.z(), world.z()));
                    }
                }
            }
        }
    }

    if (!initialized) {
        m_bboxMin = QVector3D(-1.0f, -1.0f, -1.0f);
        m_bboxMax = QVector3D(1.0f, 1.0f, 1.0f);
    } else {
        m_bboxMin = sceneMin;
        m_bboxMax = sceneMax;
    }

    m_bboxCenter = (m_bboxMin + m_bboxMax) * 0.5f;
    m_bboxRadius = qMax(0.001f, (m_bboxMax - m_bboxCenter).length());
}

QMatrix4x4 GLViewport::modelMatrix(const SceneModel &model) const
{
    QMatrix4x4 matrix;
    matrix.translate(model.translation);
    matrix.rotate(model.rotationEuler.x(), 1.0f, 0.0f, 0.0f);
    matrix.rotate(model.rotationEuler.y(), 0.0f, 1.0f, 0.0f);
    matrix.rotate(model.rotationEuler.z(), 0.0f, 0.0f, 1.0f);
    matrix.scale(model.scale);
    return matrix;
}

void GLViewport::updateSceneStats()
{
    int totalVertices = 0;
    int totalFaces = 0;
    for (const SceneModel &model : m_models) {
        totalVertices += model.vertexCount;
        totalFaces += model.faceCount;
    }
    m_vertexCount = totalVertices;
    m_faceCount = totalFaces;
    emit modelStatsChanged(m_vertexCount, m_faceCount);
}
