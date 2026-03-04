#pragma once

#include <QMainWindow>

class QAction;
class GLViewport;
class QLabel;
class QSlider;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupMenus();
    void setupStatusBar();
    void refreshStatusLabels();

    GLViewport *m_viewport{nullptr};

    QAction *m_importAction{nullptr};
    QAction *m_orbitCameraAction{nullptr};
    QAction *m_flyCameraAction{nullptr};
    QAction *m_solidShadingAction{nullptr};
    QAction *m_wireframeShadingAction{nullptr};
    QAction *m_solidWireOverlayAction{nullptr};
    QAction *m_pointCloudAction{nullptr};
    QAction *m_pointColorVertexAction{nullptr};
    QAction *m_pointColorHeightAction{nullptr};
    QSlider *m_pointSizeSlider{nullptr};

    QLabel *m_cameraStatusLabel{nullptr};
    QLabel *m_meshStatsLabel{nullptr};
    QLabel *m_shadingStatusLabel{nullptr};

    int m_vertexCount{0};
    int m_faceCount{0};
};
