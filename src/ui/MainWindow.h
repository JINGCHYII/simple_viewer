#pragma once

#include "../render/GLViewport.h"
#include "../scene/SceneManager.h"

#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTreeWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onImportModel();
    void onTreeItemChanged(QTreeWidgetItem* item, int column);
    void onTreeSelectionChanged();
    void onTreeContextMenuRequested(const QPoint& pos);
    void onDeleteSelectedObject();

private:
    void createUi();
    void createMenus();
    void applyAppStyle();
    void appendLog(const QString& message, bool isError = false);
    void refreshTree();
    void showSelectedTransform(SceneObject* object);
    SceneObject* selectedObject() const;

    SceneManager m_sceneManager;
    GLViewport* m_viewport {nullptr};
    QTreeWidget* m_treeWidget {nullptr};
    QPlainTextEdit* m_logEdit {nullptr};

    QLabel* m_idValue {nullptr};
    QLabel* m_nameValue {nullptr};
    QLabel* m_tValue {nullptr};
    QLabel* m_rValue {nullptr};
    QLabel* m_sValue {nullptr};

    bool m_syncingTree {false};
};
