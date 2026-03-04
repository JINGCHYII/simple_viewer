#pragma once

#include <QElapsedTimer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QPoint>
#include <QTimer>

#include "camera/FlyCamera.h"
#include "camera/OrbitCamera.h"
#include "render/Mesh.h"
#include "render/PointCloud.h"
#include "render/Shader.h"

class GLViewport : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    enum class CameraMode {
        Orbit,
        Fly
    };

    enum class RenderMode {
        Solid,
        Wireframe,
        SolidWireOverlay,
        PointCloud,
    };

    enum class PointColorMode {
        VertexColor,
        HeightRamp,
    };

    explicit GLViewport(QWidget *parent = nullptr);
    ~GLViewport() override;

    void setCameraMode(CameraMode mode);
    CameraMode cameraMode() const;

    void setRenderMode(RenderMode mode);
    RenderMode renderMode() const;

    void setPointSize(float size);
    float pointSize() const;

    void setPointColorMode(PointColorMode mode);
    PointColorMode pointColorMode() const;

    int vertexCount() const;
    int faceCount() const;

    void frameAll();
    void resetCamera();
    void setFrontView();
    void setRightView();
    void setTopView();

    bool loadModel(const QString &path);

signals:
    void cameraModeChanged(GLViewport::CameraMode mode);
    void renderModeChanged(GLViewport::RenderMode mode);
    void modelStatsChanged(int vertexCount, int faceCount);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Camera *currentCamera();
    const Camera *currentCamera() const;
    void uploadDefaultMesh();

    QPoint m_lastMousePos;
    OrbitCamera m_orbitCamera;
    FlyCamera m_flyCamera;
    CameraMode m_cameraMode{CameraMode::Orbit};
    RenderMode m_renderMode{RenderMode::Solid};
    PointColorMode m_pointColorMode{PointColorMode::VertexColor};
    float m_pointSize{3.0f};

    Mesh m_mesh;
    PointCloud m_pointCloud;
    Shader m_shader;

    QVector3D m_bboxMin{-1.0f, -1.0f, -1.0f};
    QVector3D m_bboxMax{1.0f, 1.0f, 1.0f};
    QVector3D m_bboxCenter{0.0f, 0.0f, 0.0f};
    float m_bboxRadius{1.732f};
    int m_vertexCount{0};
    int m_faceCount{0};

    QElapsedTimer m_frameTimer;
    QTimer m_updateTimer;
};
