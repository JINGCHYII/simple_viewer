#pragma once

#include <QMainWindow>

class QAction;
class GLViewport;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupMenus();

    GLViewport *m_viewport{nullptr};

    QAction *m_importAction{nullptr};
    QAction *m_orbitCameraAction{nullptr};
    QAction *m_flyCameraAction{nullptr};
    QAction *m_phongShadingAction{nullptr};
    QAction *m_wireframeShadingAction{nullptr};
    QAction *m_pointCloudAction{nullptr};
};
