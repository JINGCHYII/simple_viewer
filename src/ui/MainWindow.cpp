#include "MainWindow.h"

#include "../io/ModelImporter.h"

#include <QApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>

namespace {
constexpr int IdRole = Qt::UserRole + 1;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    createUi();
    createMenus();
    applyAppStyle();
    resize(1300, 800);
    statusBar()->showMessage("Ready");
    appendLog("Application started.");
}

void MainWindow::createUi()
{
    auto* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels({"Scene Object", "Visible"});
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeWidget->setMinimumWidth(280);

    connect(m_treeWidget, &QTreeWidget::itemChanged, this, &MainWindow::onTreeItemChanged);
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onTreeSelectionChanged);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested,
            this,
            &MainWindow::onTreeContextMenuRequested);

    m_viewport = new GLViewport(&m_sceneManager, this);

    auto* inspectorPanel = new QWidget(this);
    inspectorPanel->setMinimumWidth(280);
    auto* inspectorLayout = new QFormLayout(inspectorPanel);
    inspectorLayout->setContentsMargins(12, 12, 12, 12);
    inspectorLayout->setSpacing(8);

    m_idValue = new QLabel("-");
    m_nameValue = new QLabel("-");
    m_tValue = new QLabel("-");
    m_rValue = new QLabel("-");
    m_sValue = new QLabel("-");

    inspectorLayout->addRow("ID", m_idValue);
    inspectorLayout->addRow("Name", m_nameValue);
    inspectorLayout->addRow("T", m_tValue);
    inspectorLayout->addRow("R", m_rValue);
    inspectorLayout->addRow("S", m_sValue);

    rootLayout->addWidget(m_treeWidget, 1);
    rootLayout->addWidget(m_viewport, 3);
    rootLayout->addWidget(inspectorPanel, 1);
    setCentralWidget(central);

    auto* logDock = new QDockWidget("Log", this);
    logDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    logDock->setWidget(m_logEdit);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);

    connect(&m_sceneManager, &SceneManager::objectAdded, this, [this](SceneObject* object) {
        refreshTree();
        appendLog(QString("Imported object #%1: %2").arg(object->id()).arg(object->name()));
    });

    connect(&m_sceneManager, &SceneManager::objectRemoved, this, [this](int id) {
        refreshTree();
        m_viewport->setSelectedObjectId(-1);
        showSelectedTransform(nullptr);
        appendLog(QString("Removed object #%1").arg(id));
    });
}

void MainWindow::createMenus()
{
    auto* fileMenu = menuBar()->addMenu("File");
    auto* importAction = fileMenu->addAction("Import Model...");
    connect(importAction, &QAction::triggered, this, &MainWindow::onImportModel);

    auto* viewMenu = menuBar()->addMenu("View");
    auto* clearLogAction = viewMenu->addAction("Clear Log");
    connect(clearLogAction, &QAction::triggered, this, [this]() {
        m_logEdit->clear();
        appendLog("Log cleared.");
    });
}

void MainWindow::applyAppStyle()
{
    qApp->setStyleSheet(R"(
        QWidget {
            background-color: #1e1f24;
            color: #dce0e8;
            font-family: "Segoe UI", "Microsoft YaHei";
            font-size: 13px;
        }
        QMenuBar, QMenu {
            background-color: #252730;
            color: #e9edf7;
        }
        QMenu::item:selected {
            background-color: #3f6edb;
        }
        QTreeWidget, QPlainTextEdit {
            background-color: #171920;
            border: 1px solid #2f3442;
            border-radius: 6px;
            selection-background-color: #375a9e;
        }
        QHeaderView::section {
            background-color: #2a2d36;
            border: none;
            padding: 5px;
        }
        QDockWidget::title {
            text-align: center;
            background-color: #2a2d36;
            padding: 4px;
        }
        QLabel {
            color: #e6ebf5;
        }
    )");
}

void MainWindow::appendLog(const QString& message, bool isError)
{
    const QString prefix = isError ? "[ERROR]" : "[INFO]";
    m_logEdit->appendPlainText(QString("%1 %2").arg(prefix).arg(message));
}

void MainWindow::onImportModel()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Import Model",
        {},
        "Model Files (*.ply *.stl *.obj);;All Files (*)");

    if (path.isEmpty()) {
        return;
    }

    const ImportResult result = ModelImporter::importModel(path);
    if (!result.success) {
        appendLog(QString("Failed to import %1: %2").arg(path).arg(result.message), true);
        statusBar()->showMessage("Import failed", 3000);
        return;
    }

    m_sceneManager.addObject(result.objectName, path);
    statusBar()->showMessage("Import succeeded", 2000);
}

void MainWindow::refreshTree()
{
    m_syncingTree = true;
    m_treeWidget->clear();

    for (SceneObject* object : m_sceneManager.objects()) {
        auto* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, object->name());
        item->setData(0, IdRole, object->id());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setCheckState(1, object->isVisible() ? Qt::Checked : Qt::Unchecked);
        item->setText(1, object->isVisible() ? "Yes" : "No");
    }

    m_syncingTree = false;
    m_viewport->update();
}

void MainWindow::onTreeItemChanged(QTreeWidgetItem* item, int column)
{
    if (m_syncingTree || column != 1 || !item) {
        return;
    }

    const int id = item->data(0, IdRole).toInt();
    SceneObject* object = m_sceneManager.objectById(id);
    if (!object) {
        return;
    }

    const bool visible = item->checkState(1) == Qt::Checked;
    object->setVisible(visible);
    item->setText(1, visible ? "Yes" : "No");
    appendLog(QString("Object #%1 visibility -> %2").arg(id).arg(visible ? "Visible" : "Hidden"));
    m_viewport->update();
}

void MainWindow::onTreeSelectionChanged()
{
    SceneObject* object = selectedObject();
    showSelectedTransform(object);
    m_viewport->setSelectedObjectId(object ? object->id() : -1);
}

void MainWindow::showSelectedTransform(SceneObject* object)
{
    if (!object) {
        m_idValue->setText("-");
        m_nameValue->setText("-");
        m_tValue->setText("-");
        m_rValue->setText("-");
        m_sValue->setText("-");
        return;
    }

    const Transform& tr = object->transform();
    m_idValue->setText(QString::number(object->id()));
    m_nameValue->setText(object->name());
    m_tValue->setText(QString("(%1, %2, %3)")
                          .arg(tr.translation.x(), 0, 'f', 2)
                          .arg(tr.translation.y(), 0, 'f', 2)
                          .arg(tr.translation.z(), 0, 'f', 2));
    m_rValue->setText(QString("(%1, %2, %3)")
                          .arg(tr.rotationEuler.x(), 0, 'f', 2)
                          .arg(tr.rotationEuler.y(), 0, 'f', 2)
                          .arg(tr.rotationEuler.z(), 0, 'f', 2));
    m_sValue->setText(QString("(%1, %2, %3)")
                          .arg(tr.scale.x(), 0, 'f', 2)
                          .arg(tr.scale.y(), 0, 'f', 2)
                          .arg(tr.scale.z(), 0, 'f', 2));
}

SceneObject* MainWindow::selectedObject() const
{
    const QList<QTreeWidgetItem*> selected = m_treeWidget->selectedItems();
    if (selected.isEmpty()) {
        return nullptr;
    }

    const int id = selected.first()->data(0, IdRole).toInt();
    return m_sceneManager.objectById(id);
}

void MainWindow::onTreeContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) {
        return;
    }

    m_treeWidget->setCurrentItem(item);

    QMenu menu(this);
    QAction* deleteAction = menu.addAction("Delete");
    QAction* chosen = menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
    if (chosen == deleteAction) {
        onDeleteSelectedObject();
    }
}

void MainWindow::onDeleteSelectedObject()
{
    SceneObject* object = selectedObject();
    if (!object) {
        return;
    }
    m_sceneManager.removeObject(object->id());
}
