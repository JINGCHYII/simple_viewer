#pragma once

#include "../scene/SceneManager.h"

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class GLViewport : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GLViewport(SceneManager* sceneManager, QWidget* parent = nullptr);

    void setSelectedObjectId(int objectId);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    SceneManager* m_sceneManager;
    int m_selectedObjectId {-1};
};
