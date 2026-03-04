#include "app/MainWindow.h"

#include <QAction>
#include <QActionGroup>
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

    auto *shadingMenu = viewMenu->addMenu(tr("Shading"));
    auto *shadingGroup = new QActionGroup(this);

    m_phongShadingAction = shadingMenu->addAction(tr("Phong"));
    m_phongShadingAction->setCheckable(true);
    m_phongShadingAction->setChecked(true);
    shadingGroup->addAction(m_phongShadingAction);

    m_wireframeShadingAction = shadingMenu->addAction(tr("Wireframe"));
    m_wireframeShadingAction->setCheckable(true);
    shadingGroup->addAction(m_wireframeShadingAction);

    m_pointCloudAction = viewMenu->addAction(tr("Point Cloud"));
    m_pointCloudAction->setCheckable(true);
}
