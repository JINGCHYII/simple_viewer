#pragma once

#include <QElapsedTimer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QPoint>
#include <QTimer>

#include "camera/FlyCamera.h"
#include "camera/OrbitCamera.h"
#include "render/Mesh.h"
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
    };

    explicit GLViewport(QWidget *parent = nullptr);
    ~GLViewport() override;

    void setCameraMode(CameraMode mode);
    CameraMode cameraMode() const;

    void setRenderMode(RenderMode mode);
    RenderMode renderMode() const;

    bool loadModel(const QString &path);

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

    Mesh m_mesh;
    Shader m_shader;

    QElapsedTimer m_frameTimer;
    QTimer m_updateTimer;
};
