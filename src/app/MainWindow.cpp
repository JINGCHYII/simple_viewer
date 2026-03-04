#include "app/MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>

#include "render/GLViewport.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_viewport = new GLViewport(this);
    setCentralWidget(m_viewport);
    setupMenus();
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

    connect(m_solidShadingAction, &QAction::triggered, this, [this]() {
        m_viewport->setRenderMode(GLViewport::RenderMode::Solid);
    });
    connect(m_wireframeShadingAction, &QAction::triggered, this, [this]() {
        m_viewport->setRenderMode(GLViewport::RenderMode::Wireframe);
    });
    connect(m_solidWireOverlayAction, &QAction::triggered, this, [this]() {
        m_viewport->setRenderMode(GLViewport::RenderMode::SolidWireOverlay);
    });

    m_pointCloudAction = viewMenu->addAction(tr("Point Cloud"));
    m_pointCloudAction->setCheckable(true);
}
