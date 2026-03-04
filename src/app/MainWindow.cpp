#include "app/MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QSlider>
#include <QStatusBar>
#include <QWidgetAction>

#include "render/GLViewport.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_viewport = new GLViewport(this);
    setCentralWidget(m_viewport);
    setupMenus();
    setupStatusBar();

    m_vertexCount = m_viewport->vertexCount();
    m_faceCount = m_viewport->faceCount();

    connect(m_viewport, &GLViewport::cameraModeChanged, this, [this](GLViewport::CameraMode) {
        refreshStatusLabels();
    });
    connect(m_viewport, &GLViewport::renderModeChanged, this, [this](GLViewport::RenderMode) {
        refreshStatusLabels();
    });
    connect(m_viewport, &GLViewport::modelStatsChanged, this, [this](int vertexCount, int faceCount) {
        m_vertexCount = vertexCount;
        m_faceCount = faceCount;
        refreshStatusLabels();
    });

    refreshStatusLabels();
}

void MainWindow::setupMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    m_importAction = fileMenu->addAction(tr("&Import"));
    connect(m_importAction, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this,
                                                          tr("Import mesh"),
                                                          QString(),
                                                          tr("Mesh Files (*.stl *.ply)"));
        if (!path.isEmpty()) {
            m_viewport->loadModel(path);
        }
    });

    auto *viewMenu = menuBar()->addMenu(tr("&View"));

    auto *cameraMenu = viewMenu->addMenu(tr("Camera Mode"));
    auto *cameraGroup = new QActionGroup(this);

    m_orbitCameraAction = cameraMenu->addAction(tr("Orbit"));
    m_orbitCameraAction->setCheckable(true);
    m_orbitCameraAction->setChecked(true);
    cameraGroup->addAction(m_orbitCameraAction);

    m_flyCameraAction = cameraMenu->addAction(tr("Fly"));
    m_flyCameraAction->setCheckable(true);
    cameraGroup->addAction(m_flyCameraAction);

    connect(m_orbitCameraAction, &QAction::triggered, this, [this]() {
        m_viewport->setCameraMode(GLViewport::CameraMode::Orbit);
    });
    connect(m_flyCameraAction, &QAction::triggered, this, [this]() {
        m_viewport->setCameraMode(GLViewport::CameraMode::Fly);
    });

    auto *shadingMenu = viewMenu->addMenu(tr("Shading"));
    auto *shadingGroup = new QActionGroup(this);

    m_solidShadingAction = shadingMenu->addAction(tr("Solid"));
    m_solidShadingAction->setCheckable(true);
    m_solidShadingAction->setChecked(true);
    shadingGroup->addAction(m_solidShadingAction);

    m_wireframeShadingAction = shadingMenu->addAction(tr("Wireframe"));
    m_wireframeShadingAction->setCheckable(true);
    shadingGroup->addAction(m_wireframeShadingAction);

    m_solidWireOverlayAction = shadingMenu->addAction(tr("Solid + Wireframe Overlay"));
    m_solidWireOverlayAction->setCheckable(true);
    shadingGroup->addAction(m_solidWireOverlayAction);

    m_pointCloudAction = shadingMenu->addAction(tr("Point Cloud"));
    m_pointCloudAction->setCheckable(true);
    shadingGroup->addAction(m_pointCloudAction);

    connect(m_solidShadingAction, &QAction::triggered, this, [this]() {
        m_viewport->setRenderMode(GLViewport::RenderMode::Solid);
    });
    connect(m_wireframeShadingAction, &QAction::triggered, this, [this]() {
        m_viewport->setRenderMode(GLViewport::RenderMode::Wireframe);
    });
    connect(m_solidWireOverlayAction, &QAction::triggered, this, [this]() {
        m_viewport->setRenderMode(GLViewport::RenderMode::SolidWireOverlay);
    });
    connect(m_pointCloudAction, &QAction::triggered, this, [this]() {
        m_viewport->setRenderMode(GLViewport::RenderMode::PointCloud);
    });

    auto *shortcutsMenu = viewMenu->addMenu(tr("Quick Views"));
    shortcutsMenu->addAction(tr("Frame All (F)"), m_viewport, &GLViewport::frameAll);
    shortcutsMenu->addAction(tr("Front (1)"), m_viewport, &GLViewport::setFrontView);
    shortcutsMenu->addAction(tr("Right (3)"), m_viewport, &GLViewport::setRightView);
    shortcutsMenu->addAction(tr("Top (7)"), m_viewport, &GLViewport::setTopView);
    shortcutsMenu->addAction(tr("Reset Camera (R)"), m_viewport, &GLViewport::resetCamera);

    auto *pointMenu = viewMenu->addMenu(tr("Point Cloud Settings"));

    auto *pointSizeAction = new QWidgetAction(this);
    m_pointSizeSlider = new QSlider(Qt::Horizontal, this);
    m_pointSizeSlider->setRange(1, 20);
    m_pointSizeSlider->setValue(static_cast<int>(m_viewport->pointSize()));
    pointSizeAction->setDefaultWidget(m_pointSizeSlider);
    pointMenu->addAction(pointSizeAction);
    connect(m_pointSizeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_viewport->setPointSize(static_cast<float>(value));
    });

    auto *pointColorGroup = new QActionGroup(this);
    m_pointColorVertexAction = pointMenu->addAction(tr("Point Color: Vertex"));
    m_pointColorVertexAction->setCheckable(true);
    m_pointColorVertexAction->setChecked(true);
    pointColorGroup->addAction(m_pointColorVertexAction);

    m_pointColorHeightAction = pointMenu->addAction(tr("Point Color: Height Ramp"));
    m_pointColorHeightAction->setCheckable(true);
    pointColorGroup->addAction(m_pointColorHeightAction);

    connect(m_pointColorVertexAction, &QAction::triggered, this, [this]() {
        m_viewport->setPointColorMode(GLViewport::PointColorMode::VertexColor);
    });
    connect(m_pointColorHeightAction, &QAction::triggered, this, [this]() {
        m_viewport->setPointColorMode(GLViewport::PointColorMode::HeightRamp);
    });
}

void MainWindow::setupStatusBar()
{
    m_cameraStatusLabel = new QLabel(this);
    m_meshStatsLabel = new QLabel(this);
    m_shadingStatusLabel = new QLabel(this);

    statusBar()->addPermanentWidget(m_cameraStatusLabel);
    statusBar()->addPermanentWidget(m_meshStatsLabel);
    statusBar()->addPermanentWidget(m_shadingStatusLabel);
}

void MainWindow::refreshStatusLabels()
{
    const QString cameraModeText = m_viewport->cameraMode() == GLViewport::CameraMode::Orbit ? tr("Orbit") : tr("Fly");
    QString renderModeText;
    switch (m_viewport->renderMode()) {
    case GLViewport::RenderMode::Solid:
        renderModeText = tr("Solid");
        break;
    case GLViewport::RenderMode::Wireframe:
        renderModeText = tr("Wireframe");
        break;
    case GLViewport::RenderMode::SolidWireOverlay:
        renderModeText = tr("Solid+Wire");
        break;
    case GLViewport::RenderMode::PointCloud:
        renderModeText = tr("PointCloud");
        break;
    }

    m_cameraStatusLabel->setText(tr("Camera: %1").arg(cameraModeText));
    m_meshStatsLabel->setText(tr("Vertices: %1  Faces: %2").arg(m_vertexCount).arg(m_faceCount));
    m_shadingStatusLabel->setText(tr("Shading: %1").arg(renderModeText));

    m_orbitCameraAction->setChecked(m_viewport->cameraMode() == GLViewport::CameraMode::Orbit);
    m_flyCameraAction->setChecked(m_viewport->cameraMode() == GLViewport::CameraMode::Fly);

    m_solidShadingAction->setChecked(m_viewport->renderMode() == GLViewport::RenderMode::Solid);
    m_wireframeShadingAction->setChecked(m_viewport->renderMode() == GLViewport::RenderMode::Wireframe);
    m_solidWireOverlayAction->setChecked(m_viewport->renderMode() == GLViewport::RenderMode::SolidWireOverlay);
    m_pointCloudAction->setChecked(m_viewport->renderMode() == GLViewport::RenderMode::PointCloud);
}
