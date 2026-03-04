#include "render/GLViewport.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include <QFileInfo>
#include <QKeyEvent>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVector2D>
#include <QtMath>

#include "io/ModelData.h"
#include "io/ModelLoader.h"

namespace {
constexpr float kGizmoPixelSize = 120.0f;
constexpr float kGizmoLineWidth = 3.0f;
constexpr float kGizmoHoverLineWidth = 5.0f;
constexpr float kGizmoHoverBandLineWidth = 9.0f;
constexpr float kGizmoPickTolerancePx = 10.0f;
constexpr float kGizmoRotatePickBandWidthPx = 12.0f;
constexpr float kGizmoDragStartThresholdPx = 4.0f;
}

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

void GLViewport::setGizmoMode(GizmoMode mode)
{
    if (m_gizmoMode == mode) {
        return;
    }

    if (mode == GizmoMode::None) {
        m_hoveredGizmoAxis = GizmoAxis::None;
        m_pressedGizmoAxis = GizmoAxis::None;
        m_activeGizmoAxis = GizmoAxis::None;
        m_gizmoDragging = false;
    }

    m_gizmoMode = mode;
    emit gizmoModeChanged(mode);
    update();
}

GLViewport::GizmoMode GLViewport::gizmoMode() const
{
    return m_gizmoMode;
}

void GLViewport::setGizmoSpace(GizmoSpace space)
{
    if (m_gizmoSpace == space) {
        return;
    }

    m_gizmoSpace = space;
    emit gizmoSpaceChanged(space);
    update();
}

GLViewport::GizmoSpace GLViewport::gizmoSpace() const
{
    return m_gizmoSpace;
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
        emit logMessage(tr("Import failed: %1").arg(path), true);
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
        emit logMessage(tr("Import failed (no usable vertices): %1").arg(path), true);
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

    m_models.push_back(std::move(model));
    if (m_selectedModelId < 0) {
        m_selectedModelId = m_models.back().id;
        emit selectedModelChanged(m_selectedModelId);
    }

    recomputeSceneBounds();
    updateSceneStats();
    frameAll();
    emit modelListChanged();
    emit logMessage(tr("Model imported: %1").arg(path), false);
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
    emit modelListChanged();
    if (m_selectedModelId == modelId) {
        emit selectedModelChanged(modelId);
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
        emit logMessage(tr("Model removed: %1").arg(removedName), false);
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
    emit modelListChanged();
    if (m_selectedModelId == modelId) {
        emit selectedModelChanged(modelId);
    }
    update();
    return true;
}

void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glClearColor(0.18f, 0.18f, 0.18f, 1.0f);

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

    const QVector3D viewPos = currentCamera()->position();
    const QVector3D lightPos = viewPos + QVector3D(0.8f, 1.1f, 0.6f).normalized() * qMax(2.0f, m_bboxRadius * 2.8f);

    m_shader.bind(this);
    m_shader.setMat4(this, "uView", view);
    m_shader.setMat4(this, "uProj", proj);
    m_shader.setVec3(this, "uLightPos", lightPos);
    m_shader.setVec3(this, "uViewPos", viewPos);

    constexpr float baseAmbientStrength = 0.12f;
    constexpr float baseSpecularStrength = 0.24f;

    m_shader.setVec3(this, "uAmbientColor", QVector3D(0.90f, 0.92f, 0.95f));
    m_shader.setVec3(this, "uLightColor", QVector3D(0.94f, 0.94f, 0.93f));
    m_shader.setFloat(this, "uAmbientStrength", baseAmbientStrength);
    m_shader.setFloat(this, "uSpecularStrength", baseSpecularStrength);
    m_shader.setFloat(this, "uShininess", 48.0f);
    m_shader.setInt(this, "uUseVertexColor", 1);
    m_shader.setInt(this, "uEnableSpecular", 1);
    m_shader.setInt(this, "uShadingModel", 0);
    m_shader.setFloat(this, "uRimStrength", 0.0f);
    m_shader.setFloat(this, "uFlatFactor", 0.0f);
    m_shader.setFloat(this, "uGamma", 2.2f);
    m_shader.setFloat(this, "uPointSize", m_pointSize);
    m_shader.setInt(this, "uPointColorMode", m_pointColorMode == PointColorMode::VertexColor ? 0 : 1);

    for (const SceneModel &model : m_models) {
        if (!model.visible) {
            continue;
        }

        const bool drawMesh = model.hasMesh && model.mesh.isValid();
        const bool drawPoints = !drawMesh && model.hasPointCloud && model.pointCloud.isValid();
        if (!drawPoints && !drawMesh) {
            continue;
        }

        m_shader.setMat4(this, "uModel", modelMatrix(model));
        m_shader.setInt(this, "uIsPointRender", drawPoints ? 1 : 0);
        m_shader.setVec3(this, "uBoundsMin", model.bboxMin);
        m_shader.setVec3(this, "uBoundsMax", model.bboxMax);

        const bool selected = model.id == m_selectedModelId;
        m_shader.setVec3(this, "uMaterialColor", selected ? QVector3D(1.0f, 0.56f, 0.22f) : QVector3D(0.72f, 0.74f, 0.78f));

        if (drawPoints) {
            m_shader.setInt(this, "uEnableSpecular", 0);
            m_shader.setInt(this, "uShadingModel", 0);
            m_shader.setFloat(this, "uRimStrength", 0.0f);
            m_shader.setFloat(this, "uFlatFactor", 0.0f);
            m_shader.setFloat(this, "uGamma", 2.2f);
            m_shader.setFloat(this, "uAmbientStrength", selected ? 0.18f : 0.12f);
            m_shader.setFloat(this, "uSpecularStrength", 0.0f);
            model.pointCloud.draw(this);
            m_shader.setFloat(this, "uAmbientStrength", baseAmbientStrength);
            m_shader.setFloat(this, "uSpecularStrength", baseSpecularStrength);
        } else {
            m_shader.setInt(this, "uShadingModel", 1);
            m_shader.setFloat(this, "uAmbientStrength", selected ? 0.18f : 0.14f);
            m_shader.setFloat(this, "uSpecularStrength", selected ? 0.34f : 0.30f);
            m_shader.setFloat(this, "uShininess", 84.0f);
            m_shader.setInt(this, "uEnableSpecular", 1);
            m_shader.setFloat(this, "uRimStrength", selected ? 0.24f : 0.18f);
            m_shader.setFloat(this, "uFlatFactor", 0.30f);
            m_shader.setFloat(this, "uGamma", 1.9f);
            model.mesh.draw(this);

            if (m_renderMode == RenderMode::SolidWireOverlay) {
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0f, -1.0f);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                m_shader.setInt(this, "uUseVertexColor", 0);
                m_shader.setVec3(this, "uMaterialColor", QVector3D(0.03f, 0.03f, 0.04f));
                m_shader.setFloat(this, "uAmbientStrength", 1.0f);
                m_shader.setFloat(this, "uSpecularStrength", 0.0f);
                m_shader.setInt(this, "uEnableSpecular", 0);
                m_shader.setInt(this, "uShadingModel", 0);
                m_shader.setFloat(this, "uRimStrength", 0.0f);
                m_shader.setFloat(this, "uFlatFactor", 0.0f);
                m_shader.setFloat(this, "uGamma", 2.2f);
                model.mesh.draw(this);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDisable(GL_POLYGON_OFFSET_LINE);

                m_shader.setInt(this, "uUseVertexColor", 1);
            }

            m_shader.setFloat(this, "uAmbientStrength", baseAmbientStrength);
            m_shader.setFloat(this, "uSpecularStrength", baseSpecularStrength);
            m_shader.setFloat(this, "uShininess", 48.0f);
            m_shader.setInt(this, "uEnableSpecular", 1);
            m_shader.setInt(this, "uShadingModel", 0);
            m_shader.setFloat(this, "uRimStrength", 0.0f);
            m_shader.setFloat(this, "uFlatFactor", 0.0f);
            m_shader.setFloat(this, "uGamma", 2.2f);
        }
    }

    if (m_gizmoMode != GizmoMode::None) {
        if (const SceneModel *selected = findModel(m_selectedModelId)) {
            glDisable(GL_DEPTH_TEST);
            drawGizmo(*selected, view, proj);
            glEnable(GL_DEPTH_TEST);
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
    case Qt::Key_0:
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
    m_pressedGizmoAxis = GizmoAxis::None;

    if (event->button() == Qt::LeftButton && m_selectedModelId >= 0) {
        m_hoveredGizmoAxis = pickGizmoAxis(event->pos());
        if (m_hoveredGizmoAxis != GizmoAxis::None) {
            if (SceneModel *selected = findModel(m_selectedModelId)) {
                m_gizmoDragging = false;
                m_activeGizmoAxis = GizmoAxis::None;
                m_pressedGizmoAxis = m_hoveredGizmoAxis;
                m_gizmoDragStartScreen = event->pos();
                m_gizmoStartTranslation = selected->translation;
                m_gizmoStartRotation = selected->rotationEuler;
                m_gizmoStartScale = selected->scale;
                event->accept();
                return;
            }
        }
    }

    currentCamera()->handleMousePress(event->button(), event->pos());
    event->accept();
}

void GLViewport::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selectedModelId >= 0 && !m_gizmoDragging) {
        m_hoveredGizmoAxis = pickGizmoAxis(event->pos());
    }

    const float dpiScale = qMax(1.0f, static_cast<float>(devicePixelRatioF()));
    const float dragThresholdPx = kGizmoDragStartThresholdPx * dpiScale;

    if (m_pressedGizmoAxis != GizmoAxis::None && !m_gizmoDragging) {
        const QVector2D dragDelta(event->pos() - m_gizmoDragStartScreen);
        if (dragDelta.length() >= dragThresholdPx) {
            m_gizmoDragging = true;
            m_activeGizmoAxis = m_pressedGizmoAxis;
        } else {
            event->accept();
            return;
        }
    }

    if (m_gizmoDragging && m_activeGizmoAxis != GizmoAxis::None) {
        SceneModel *selected = findModel(m_selectedModelId);
        if (selected != nullptr) {
            const QVector3D center = selectedModelWorldCenter(*selected);
            const float worldPerPixel = worldUnitsPerPixelAt(center);
            const QPoint delta = event->pos() - m_gizmoDragStartScreen;

            const QVector3D axisVector = gizmoAxisVector(m_activeGizmoAxis, *selected);

            const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
            const QMatrix4x4 viewProj = currentCamera()->projMatrix(aspect) * currentCamera()->viewMatrix();
            const QPointF centerScreen = projectToScreen(center, viewProj);
            const QPointF axisScreen = projectToScreen(center + axisVector * worldPerPixel * kGizmoPixelSize, viewProj);
            QVector2D axisDir(axisScreen - centerScreen);
            if (axisDir.lengthSquared() < 1e-4f) {
                axisDir = QVector2D(1.0f, 0.0f);
            } else {
                axisDir.normalize();
            }
            const float signedPixels = QVector2D::dotProduct(QVector2D(delta), axisDir);

            QVector3D translation = m_gizmoStartTranslation;
            QVector3D rotation = m_gizmoStartRotation;
            QVector3D scale = m_gizmoStartScale;

            if (m_gizmoMode == GizmoMode::Translate) {
                translation += axisVector * (signedPixels * worldPerPixel);
            } else if (m_gizmoMode == GizmoMode::Rotate) {
                const float rotateDelta = signedPixels * 0.35f;
                if (m_activeGizmoAxis == GizmoAxis::X) {
                    rotation.setX(m_gizmoStartRotation.x() + rotateDelta);
                } else if (m_activeGizmoAxis == GizmoAxis::Y) {
                    rotation.setY(m_gizmoStartRotation.y() + rotateDelta);
                } else if (m_activeGizmoAxis == GizmoAxis::Z) {
                    rotation.setZ(m_gizmoStartRotation.z() + rotateDelta);
                }
            } else if (m_gizmoMode == GizmoMode::Scale) {
                const float scaleFactor = qMax(0.01f, 1.0f + signedPixels * 0.01f);
                if (m_activeGizmoAxis == GizmoAxis::X) {
                    scale.setX(qMax(0.001f, m_gizmoStartScale.x() * scaleFactor));
                } else if (m_activeGizmoAxis == GizmoAxis::Y) {
                    scale.setY(qMax(0.001f, m_gizmoStartScale.y() * scaleFactor));
                } else if (m_activeGizmoAxis == GizmoAxis::Z) {
                    scale.setZ(qMax(0.001f, m_gizmoStartScale.z() * scaleFactor));
                }
            }

            setModelTransform(m_selectedModelId, translation, rotation, scale);
        }

        m_lastMousePos = event->pos();
        event->accept();
        return;
    }

    m_lastMousePos = event->pos();
    currentCamera()->handleMouseMove(event->pos());
    event->accept();
    update();
}

void GLViewport::mouseReleaseEvent(QMouseEvent *event)
{
    const bool pressedOnGizmo = m_pressedGizmoAxis != GizmoAxis::None;
    m_gizmoDragging = false;
    m_pressedGizmoAxis = GizmoAxis::None;
    m_activeGizmoAxis = GizmoAxis::None;

    if (!pressedOnGizmo) {
        currentCamera()->handleMouseRelease(event->button(), event->pos());
    }
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

QVector3D GLViewport::selectedModelWorldCenter(const SceneModel &model) const
{
    const QVector3D localCenter = (model.bboxMin + model.bboxMax) * 0.5f;
    return (modelMatrix(model) * QVector4D(localCenter, 1.0f)).toVector3D();
}

float GLViewport::worldUnitsPerPixelAt(const QVector3D &worldPoint) const
{
    const float distance = qMax(0.1f, (currentCamera()->position() - worldPoint).length());
    const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    const QMatrix4x4 proj = currentCamera()->projMatrix(aspect);
    const float projY = qMax(0.0001f, std::abs(proj(1, 1)));
    return (2.0f * distance) / (qMax(1, height()) * projY);
}

QPointF GLViewport::projectToScreen(const QVector3D &worldPoint, const QMatrix4x4 &viewProj) const
{
    const QVector4D clip = viewProj * QVector4D(worldPoint, 1.0f);
    if (qFuzzyIsNull(clip.w())) {
        return QPointF(-10000.0, -10000.0);
    }
    const QVector3D ndc = (clip / clip.w()).toVector3D();
    const float sx = (ndc.x() * 0.5f + 0.5f) * static_cast<float>(width());
    const float sy = (1.0f - (ndc.y() * 0.5f + 0.5f)) * static_cast<float>(height());
    return QPointF(sx, sy);
}

QVector3D GLViewport::gizmoAxisVector(GizmoAxis axis, const SceneModel &model) const
{
    QVector3D baseAxis;
    switch (axis) {
    case GizmoAxis::X:
        baseAxis = QVector3D(1.0f, 0.0f, 0.0f);
        break;
    case GizmoAxis::Y:
        baseAxis = QVector3D(0.0f, 1.0f, 0.0f);
        break;
    case GizmoAxis::Z:
        baseAxis = QVector3D(0.0f, 0.0f, 1.0f);
        break;
    case GizmoAxis::None:
    default:
        return QVector3D(0.0f, 0.0f, 0.0f);
    }

    if (m_gizmoSpace == GizmoSpace::World) {
        return baseAxis;
    }

    const QMatrix4x4 modelMat = modelMatrix(model);
    QVector3D localInWorld = (modelMat * QVector4D(baseAxis, 0.0f)).toVector3D();
    if (localInWorld.lengthSquared() < 1e-6f) {
        return baseAxis;
    }
    return localInWorld.normalized();
}


GLViewport::GizmoAxis GLViewport::pickGizmoAxis(const QPoint &screenPos) const
{
    if (m_gizmoMode == GizmoMode::None || m_selectedModelId < 0) {
        return GizmoAxis::None;
    }
    const SceneModel *selected = findModel(m_selectedModelId);
    if (selected == nullptr) {
        return GizmoAxis::None;
    }

    const QVector3D center = selectedModelWorldCenter(*selected);
    const float worldPerPixel = worldUnitsPerPixelAt(center);
    const float axisLen = worldPerPixel * kGizmoPixelSize;

    const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    const QMatrix4x4 viewProj = currentCamera()->projMatrix(aspect) * currentCamera()->viewMatrix();

    const QPointF centerPt = projectToScreen(center, viewProj);
    const std::array<GizmoAxis, 3> picks = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};

    const float dpiScale = qMax(1.0f, static_cast<float>(devicePixelRatioF()));

    const auto pointToSegmentDistance = [](const QPointF &point, const QPointF &a, const QPointF &b) {
        const QVector2D segment(b - a);
        const float segLenSq = segment.lengthSquared();
        if (segLenSq < 1e-6f) {
            return QVector2D(point - a).length();
        }
        const float t = qBound(0.0f, QVector2D::dotProduct(QVector2D(point - a), segment) / segLenSq, 1.0f);
        const QPointF nearest = a + (b - a) * t;
        return QVector2D(point - nearest).length();
    };

    GizmoAxis bestAxis = GizmoAxis::None;
    float bestDist = std::numeric_limits<float>::max();
    for (GizmoAxis axis : picks) {
        const QVector3D axisDir = gizmoAxisVector(axis, *selected);
        if (m_gizmoMode == GizmoMode::Rotate) {
            constexpr int segments = 72;
            QVector3D u = QVector3D::crossProduct(axisDir, QVector3D(0.0f, 1.0f, 0.0f));
            if (u.lengthSquared() < 1e-4f) {
                u = QVector3D::crossProduct(axisDir, QVector3D(1.0f, 0.0f, 0.0f));
            }
            u.normalize();
            const QVector3D v = QVector3D::crossProduct(axisDir, u).normalized();

            float ringDist = std::numeric_limits<float>::max();
            QPointF prevPoint;
            for (int i = 0; i <= segments; ++i) {
                const float angle = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * 3.14159265f;
                const QVector3D ringPoint = center + (u * qCos(angle) + v * qSin(angle)) * (axisLen * 0.75f);
                const QPointF currPoint = projectToScreen(ringPoint, viewProj);
                if (i > 0) {
                    ringDist = qMin(ringDist, pointToSegmentDistance(QPointF(screenPos), prevPoint, currPoint));
                }
                prevPoint = currPoint;
            }

            const float ringHalfBand = 0.5f * kGizmoRotatePickBandWidthPx * dpiScale;
            if (ringDist <= ringHalfBand && ringDist < bestDist) {
                bestDist = ringDist;
                bestAxis = axis;
            }
        } else {
            const QPointF endPt = projectToScreen(center + axisDir * axisLen, viewProj);
            const float dist = pointToSegmentDistance(QPointF(screenPos), centerPt, endPt);
            if (dist <= kGizmoPickTolerancePx * dpiScale && dist < bestDist) {
                bestDist = dist;
                bestAxis = axis;
            }
        }
    }
    return bestAxis;
}

void GLViewport::drawGizmo(const SceneModel &model, const QMatrix4x4 &view, const QMatrix4x4 &proj)
{
    if (m_gizmoMode == GizmoMode::None) {
        return;
    }

    const QVector3D center = selectedModelWorldCenter(model);
    const float axisLen = worldUnitsPerPixelAt(center) * kGizmoPixelSize;
    const float circleRadius = axisLen * 0.75f;

    const QVector3D colorX = (m_activeGizmoAxis == GizmoAxis::X || m_hoveredGizmoAxis == GizmoAxis::X) ? QVector3D(1.0f, 0.85f, 0.3f) : QVector3D(0.96f, 0.26f, 0.26f);
    const QVector3D colorY = (m_activeGizmoAxis == GizmoAxis::Y || m_hoveredGizmoAxis == GizmoAxis::Y) ? QVector3D(1.0f, 0.85f, 0.3f) : QVector3D(0.22f, 0.9f, 0.22f);
    const QVector3D colorZ = (m_activeGizmoAxis == GizmoAxis::Z || m_hoveredGizmoAxis == GizmoAxis::Z) ? QVector3D(1.0f, 0.85f, 0.3f) : QVector3D(0.25f, 0.52f, 0.96f);

    const QVector3D axisX = gizmoAxisVector(GizmoAxis::X, model);
    const QVector3D axisY = gizmoAxisVector(GizmoAxis::Y, model);
    const QVector3D axisZ = gizmoAxisVector(GizmoAxis::Z, model);

    if (m_gizmoMode == GizmoMode::Rotate) {
        if (m_hoveredGizmoAxis != GizmoAxis::None) {
            const QVector3D hoverAxis = gizmoAxisVector(m_hoveredGizmoAxis, model);
            const QVector3D hoverBandColor(0.95f, 0.85f, 0.35f);
            drawGizmoCircle(center, hoverAxis, circleRadius, hoverBandColor, view, proj, kGizmoHoverBandLineWidth);
        }
        drawGizmoCircle(center, axisX, circleRadius, colorX, view, proj,
                        m_hoveredGizmoAxis == GizmoAxis::X || m_activeGizmoAxis == GizmoAxis::X ? kGizmoHoverLineWidth : kGizmoLineWidth);
        drawGizmoCircle(center, axisY, circleRadius, colorY, view, proj,
                        m_hoveredGizmoAxis == GizmoAxis::Y || m_activeGizmoAxis == GizmoAxis::Y ? kGizmoHoverLineWidth : kGizmoLineWidth);
        drawGizmoCircle(center, axisZ, circleRadius, colorZ, view, proj,
                        m_hoveredGizmoAxis == GizmoAxis::Z || m_activeGizmoAxis == GizmoAxis::Z ? kGizmoHoverLineWidth : kGizmoLineWidth);
    } else {
        drawGizmoLine(center, center + axisX * axisLen, colorX, view, proj);
        drawGizmoLine(center, center + axisY * axisLen, colorY, view, proj);
        drawGizmoLine(center, center + axisZ * axisLen, colorZ, view, proj);
    }
}

void GLViewport::drawGizmoLine(const QVector3D &start, const QVector3D &end, const QVector3D &color,
                               const QMatrix4x4 &view, const QMatrix4x4 &proj)
{
    std::vector<Vertex> vertices = {
        {{start.x(), start.y(), start.z()}, {0.0f, 1.0f, 0.0f}, {color.x(), color.y(), color.z()}},
        {{end.x(), end.y(), end.z()}, {0.0f, 1.0f, 0.0f}, {color.x(), color.y(), color.z()}},
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, color)));

    m_shader.setMat4(this, "uModel", QMatrix4x4());
    m_shader.setMat4(this, "uView", view);
    m_shader.setMat4(this, "uProj", proj);
    m_shader.setInt(this, "uUseVertexColor", 1);
    m_shader.setInt(this, "uEnableSpecular", 0);
    m_shader.setInt(this, "uShadingModel", 0);
    glLineWidth(kGizmoLineWidth);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    glLineWidth(1.0f);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void GLViewport::drawGizmoCircle(const QVector3D &center, const QVector3D &axis, float radius, const QVector3D &color,
                                 const QMatrix4x4 &view, const QMatrix4x4 &proj, float lineWidth)
{
    constexpr int segments = 48;
    QVector3D u = QVector3D::crossProduct(axis, QVector3D(0.0f, 1.0f, 0.0f));
    if (u.lengthSquared() < 1e-4f) {
        u = QVector3D::crossProduct(axis, QVector3D(1.0f, 0.0f, 0.0f));
    }
    u.normalize();
    QVector3D v = QVector3D::crossProduct(axis, u).normalized();

    std::vector<Vertex> vertices;
    vertices.reserve(segments * 2);
    for (int i = 0; i < segments; ++i) {
        const float a0 = (static_cast<float>(i) / segments) * 2.0f * 3.14159265f;
        const float a1 = (static_cast<float>(i + 1) / segments) * 2.0f * 3.14159265f;
        const QVector3D p0 = center + (u * qCos(a0) + v * qSin(a0)) * radius;
        const QVector3D p1 = center + (u * qCos(a1) + v * qSin(a1)) * radius;
        vertices.push_back({{p0.x(), p0.y(), p0.z()}, {axis.x(), axis.y(), axis.z()}, {color.x(), color.y(), color.z()}});
        vertices.push_back({{p1.x(), p1.y(), p1.z()}, {axis.x(), axis.y(), axis.z()}, {color.x(), color.y(), color.z()}});
    }

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, color)));

    m_shader.setMat4(this, "uModel", QMatrix4x4());
    m_shader.setMat4(this, "uView", view);
    m_shader.setMat4(this, "uProj", proj);
    m_shader.setInt(this, "uUseVertexColor", 1);
    m_shader.setInt(this, "uEnableSpecular", 0);
    m_shader.setInt(this, "uShadingModel", 0);
    glLineWidth(lineWidth);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    glLineWidth(1.0f);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
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
    model.name = tr("Default cube");
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
