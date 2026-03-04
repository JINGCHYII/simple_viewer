#pragma once

#include <QMainWindow>

class QAction;
class GLViewport;
class QLabel;
class QSlider;
class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class QTextEdit;
class QDoubleSpinBox;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupMenus();
    void setupStatusBar();
    void setupPanels();
    void setupTheme();
    void refreshStatusLabels();
    void refreshModelTree();
    void refreshModelVisibilityRow(int modelId, bool visible);
    QToolButton *createVisibilityButton(int modelId, bool visible);
    void refreshTransformPanel();
    void appendLog(const QString &message, bool isError = false);

    GLViewport *m_viewport{nullptr};

    QAction *m_importAction{nullptr};
    QAction *m_orbitCameraAction{nullptr};
    QAction *m_flyCameraAction{nullptr};
    QAction *m_autoFitCameraAction{nullptr};
    QAction *m_solidWireOverlayAction{nullptr};
    QAction *m_pointColorVertexAction{nullptr};
    QAction *m_pointColorHeightAction{nullptr};
    QAction *m_gizmoHideAction{nullptr};
    QAction *m_gizmoTranslateAction{nullptr};
    QAction *m_gizmoRotateAction{nullptr};
    QAction *m_gizmoScaleAction{nullptr};
    QSlider *m_pointSizeSlider{nullptr};

    QLabel *m_cameraStatusLabel{nullptr};
    QLabel *m_meshStatsLabel{nullptr};
    QLabel *m_shadingStatusLabel{nullptr};

    QTreeWidget *m_sceneTree{nullptr};
    QTabWidget *m_propertyTabs{nullptr};
    QTextEdit *m_logView{nullptr};

    QDoubleSpinBox *m_txSpin{nullptr};
    QDoubleSpinBox *m_tySpin{nullptr};
    QDoubleSpinBox *m_tzSpin{nullptr};
    QDoubleSpinBox *m_rxSpin{nullptr};
    QDoubleSpinBox *m_rySpin{nullptr};
    QDoubleSpinBox *m_rzSpin{nullptr};
    QDoubleSpinBox *m_sxSpin{nullptr};
    QDoubleSpinBox *m_sySpin{nullptr};
    QDoubleSpinBox *m_szSpin{nullptr};

    int m_vertexCount{0};
    int m_faceCount{0};
    bool m_syncingUi{false};
};
