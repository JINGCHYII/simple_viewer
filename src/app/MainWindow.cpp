#include "app/MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QDateTime>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QIcon>
#include <QMenuBar>
#include <QSlider>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QDoubleSpinBox>

#include "render/GLViewport.h"

namespace {

QIcon visibilityIcon(bool visible)
{
    return QIcon(visible ? QStringLiteral(":/icons/eye_open.svg") : QStringLiteral(":/icons/eye_closed.svg"));
}

void setVisibilityButtonState(QToolButton *button, bool visible)
{
    if (button == nullptr) {
        return;
    }
    button->setProperty("visibleState", visible);
    button->setIcon(visibilityIcon(visible));
    button->setToolTip(visible ? QObject::tr("隐藏模型") : QObject::tr("显示模型"));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Simple Viewer Pro"));
    resize(1400, 900);

    m_viewport = new GLViewport(this);
    setCentralWidget(m_viewport);

    setupMenus();
    setupPanels();
    setupStatusBar();
    setupTheme();

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
    connect(m_viewport, &GLViewport::modelListChanged, this, [this]() {
        refreshModelTree();
        refreshTransformPanel();
    });
    connect(m_viewport, &GLViewport::selectedModelChanged, this, [this](int) {
        refreshModelTree();
        refreshTransformPanel();
    });
    connect(m_viewport, &GLViewport::logMessage, this, [this](const QString &message, bool isError) {
        appendLog(message, isError);
    });

    refreshModelTree();
    refreshTransformPanel();
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
                                                          tr("3D Files (*.obj *.fbx *.dae *.3ds *.glb *.gltf *.stl *.ply *.off *.x *.blend);;All Files (*)"));
        if (!path.isEmpty()) {
            if (!m_viewport->loadModel(path)) {
                appendLog(tr("导入失败: %1").arg(path), true);
            }
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

    m_autoFitCameraAction = viewMenu->addAction(tr("Auto Fit Camera"));
    m_autoFitCameraAction->setCheckable(true);
    m_autoFitCameraAction->setChecked(m_viewport->autoFitEnabled());
    connect(m_autoFitCameraAction, &QAction::toggled, this, [this](bool enabled) {
        m_viewport->setAutoFitEnabled(enabled);
    });

    viewMenu->addSeparator();

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

void MainWindow::setupPanels()
{
    auto *sceneDock = new QDockWidget(tr("Scene Tree"), this);
    sceneDock->setObjectName("SceneDock");
    m_sceneTree = new QTreeWidget(sceneDock);
    m_sceneTree->setHeaderLabels({tr("模型"), tr("顶点"), tr("面"), QStringLiteral(" ")});
    m_sceneTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_sceneTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_sceneTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_sceneTree->header()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_sceneTree->setColumnWidth(3, 36);
    m_sceneTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sceneTree->setContextMenuPolicy(Qt::CustomContextMenu);
    sceneDock->setWidget(m_sceneTree);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);

    connect(m_sceneTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        if (m_syncingUi || m_sceneTree->selectedItems().isEmpty()) {
            return;
        }
        const int modelId = m_sceneTree->selectedItems().front()->data(0, Qt::UserRole).toInt();
        m_viewport->selectModel(modelId);
    });
    connect(m_sceneTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QTreeWidgetItem *item = m_sceneTree->itemAt(pos);
        if (item == nullptr) {
            return;
        }
        QMenu menu(this);
        QAction *deleteAction = menu.addAction(tr("删除模型"));
        QAction *selected = menu.exec(m_sceneTree->viewport()->mapToGlobal(pos));
        if (selected == deleteAction) {
            const int modelId = item->data(0, Qt::UserRole).toInt();
            m_viewport->removeModel(modelId);
        }
    });

    auto *propertyDock = new QDockWidget(tr("Inspector"), this);
    propertyDock->setObjectName("InspectorDock");
    m_propertyTabs = new QTabWidget(propertyDock);
    auto *transformTab = new QWidget(m_propertyTabs);
    auto *formLayout = new QFormLayout(transformTab);

    auto createSpin = [this](double min, double max, double step) {
        auto *spin = new QDoubleSpinBox(this);
        spin->setRange(min, max);
        spin->setDecimals(3);
        spin->setSingleStep(step);
        return spin;
    };

    m_txSpin = createSpin(-10000.0, 10000.0, 0.1);
    m_tySpin = createSpin(-10000.0, 10000.0, 0.1);
    m_tzSpin = createSpin(-10000.0, 10000.0, 0.1);
    m_rxSpin = createSpin(-360.0, 360.0, 1.0);
    m_rySpin = createSpin(-360.0, 360.0, 1.0);
    m_rzSpin = createSpin(-360.0, 360.0, 1.0);
    m_sxSpin = createSpin(0.001, 1000.0, 0.05);
    m_sySpin = createSpin(0.001, 1000.0, 0.05);
    m_szSpin = createSpin(0.001, 1000.0, 0.05);

    formLayout->addRow(tr("Translate X"), m_txSpin);
    formLayout->addRow(tr("Translate Y"), m_tySpin);
    formLayout->addRow(tr("Translate Z"), m_tzSpin);
    formLayout->addRow(tr("Rotate X"), m_rxSpin);
    formLayout->addRow(tr("Rotate Y"), m_rySpin);
    formLayout->addRow(tr("Rotate Z"), m_rzSpin);
    formLayout->addRow(tr("Scale X"), m_sxSpin);
    formLayout->addRow(tr("Scale Y"), m_sySpin);
    formLayout->addRow(tr("Scale Z"), m_szSpin);

    auto applyTransform = [this]() {
        if (m_syncingUi) {
            return;
        }
        const int id = m_viewport->selectedModelId();
        if (id < 0) {
            return;
        }
        m_viewport->setModelTransform(id,
                                      QVector3D(m_txSpin->value(), m_tySpin->value(), m_tzSpin->value()),
                                      QVector3D(m_rxSpin->value(), m_rySpin->value(), m_rzSpin->value()),
                                      QVector3D(m_sxSpin->value(), m_sySpin->value(), m_szSpin->value()));
    };

    for (QDoubleSpinBox *spin : {m_txSpin, m_tySpin, m_tzSpin, m_rxSpin, m_rySpin, m_rzSpin, m_sxSpin, m_sySpin, m_szSpin}) {
        connect(spin, &QDoubleSpinBox::editingFinished, this, [applyTransform]() {
            applyTransform();
        });
    }

    m_propertyTabs->addTab(transformTab, tr("Transform"));
    propertyDock->setWidget(m_propertyTabs);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock);

    auto *logDock = new QDockWidget(tr("Log Console"), this);
    logDock->setObjectName("LogDock");
    m_logView = new QTextEdit(logDock);
    m_logView->setReadOnly(true);
    logDock->setWidget(m_logView);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);

    appendLog(tr("Viewer 已启动"));
}

void MainWindow::setupTheme()
{
    setStyleSheet(R"(
QMainWindow {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #111827, stop:1 #1f2937);
}
QMenuBar, QMenu, QStatusBar {
    background-color: #0f172a;
    color: #e2e8f0;
}
QDockWidget {
    color: #f8fafc;
    font-weight: 600;
}
QDockWidget::title {
    text-align: left;
    padding-left: 8px;
    background: #1e293b;
    border-bottom: 1px solid #334155;
}
QTreeWidget, QTextEdit, QTabWidget::pane, QDoubleSpinBox {
    background-color: #0b1220;
    color: #e2e8f0;
    border: 1px solid #334155;
    border-radius: 6px;
}
QTreeWidget::item:selected {
    background-color: #2563eb;
}
QTabBar::tab {
    background: #1e293b;
    color: #e2e8f0;
    padding: 6px 12px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}
QTabBar::tab:selected {
    background: #2563eb;
}
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    background: #1e293b;
    width: 16px;
}
)");
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
    m_meshStatsLabel->setText(tr("Scene Vertices: %1  Faces: %2").arg(m_vertexCount).arg(m_faceCount));
    m_shadingStatusLabel->setText(tr("Shading: %1").arg(renderModeText));

    m_orbitCameraAction->setChecked(m_viewport->cameraMode() == GLViewport::CameraMode::Orbit);
    m_flyCameraAction->setChecked(m_viewport->cameraMode() == GLViewport::CameraMode::Fly);
    m_autoFitCameraAction->setChecked(m_viewport->autoFitEnabled());

    m_solidShadingAction->setChecked(m_viewport->renderMode() == GLViewport::RenderMode::Solid);
    m_wireframeShadingAction->setChecked(m_viewport->renderMode() == GLViewport::RenderMode::Wireframe);
    m_solidWireOverlayAction->setChecked(m_viewport->renderMode() == GLViewport::RenderMode::SolidWireOverlay);
    m_pointCloudAction->setChecked(m_viewport->renderMode() == GLViewport::RenderMode::PointCloud);
}

void MainWindow::refreshModelTree()
{
    m_syncingUi = true;
    m_sceneTree->clear();

    const int selectedId = m_viewport->selectedModelId();
    for (const GLViewport::ModelInfo &info : m_viewport->modelInfos()) {
        auto *item = new QTreeWidgetItem(m_sceneTree);
        item->setText(0, info.name);
        item->setText(1, QString::number(info.vertexCount));
        item->setText(2, QString::number(info.faceCount));
        item->setData(0, Qt::UserRole, info.id);
        item->setToolTip(0, info.path);
        m_sceneTree->setItemWidget(item, 3, createVisibilityButton(info.id, info.visible));

        if (info.id == selectedId) {
            m_sceneTree->setCurrentItem(item);
        }
    }
    m_syncingUi = false;
}


void MainWindow::refreshModelVisibilityRow(int modelId, bool visible)
{
    for (int row = 0; row < m_sceneTree->topLevelItemCount(); ++row) {
        QTreeWidgetItem *item = m_sceneTree->topLevelItem(row);
        if (item == nullptr || item->data(0, Qt::UserRole).toInt() != modelId) {
            continue;
        }

        auto *button = qobject_cast<QToolButton *>(m_sceneTree->itemWidget(item, 3));
        setVisibilityButtonState(button, visible);
        return;
    }
}

QToolButton *MainWindow::createVisibilityButton(int modelId, bool visible)
{
    auto *button = new QToolButton(m_sceneTree);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(24, 24);
    button->setIconSize(QSize(16, 16));
    setVisibilityButtonState(button, visible);

    connect(button, &QToolButton::clicked, this, [this, modelId, button]() {
        if (m_syncingUi) {
            return;
        }

        const bool currentVisible = button->property("visibleState").toBool();
        const bool nextVisible = !currentVisible;
        if (m_viewport->setModelVisible(modelId, nextVisible)) {
            refreshModelVisibilityRow(modelId, nextVisible);
        }
    });

    return button;
}

void MainWindow::refreshTransformPanel()
{
    m_syncingUi = true;
    const int selectedId = m_viewport->selectedModelId();
    bool found = false;
    for (const GLViewport::ModelInfo &info : m_viewport->modelInfos()) {
        if (info.id != selectedId) {
            continue;
        }
        m_txSpin->setValue(info.translation.x());
        m_tySpin->setValue(info.translation.y());
        m_tzSpin->setValue(info.translation.z());
        m_rxSpin->setValue(info.rotationEuler.x());
        m_rySpin->setValue(info.rotationEuler.y());
        m_rzSpin->setValue(info.rotationEuler.z());
        m_sxSpin->setValue(info.scale.x());
        m_sySpin->setValue(info.scale.y());
        m_szSpin->setValue(info.scale.z());
        found = true;
        break;
    }

    for (QDoubleSpinBox *spin : {m_txSpin, m_tySpin, m_tzSpin, m_rxSpin, m_rySpin, m_rzSpin, m_sxSpin, m_sySpin, m_szSpin}) {
        spin->setEnabled(found);
    }
    m_syncingUi = false;
}

void MainWindow::appendLog(const QString &message, bool isError)
{
    const QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    const QString level = isError ? "ERROR" : "INFO";
    const QString color = isError ? "#f87171" : "#93c5fd";
    m_logView->append(QString("<span style='color:%1'>[%2][%3] %4</span>").arg(color, timestamp, level, message.toHtmlEscaped()));
}
