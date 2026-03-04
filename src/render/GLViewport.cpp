#include "GLViewport.h"

#include <QPainter>

GLViewport::GLViewport(SceneManager* sceneManager, QWidget* parent)
    : QOpenGLWidget(parent), m_sceneManager(sceneManager)
{
    setMinimumSize(640, 480);
}

void GLViewport::setSelectedObjectId(int objectId)
{
    m_selectedObjectId = objectId;
    update();
}

void GLViewport::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
}

void GLViewport::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLViewport::paintGL()
{
    glClearColor(0.10f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int y = 24;
    painter.drawText(12, y, "Viewport Preview (placeholder renderer)");
    y += 20;

    for (SceneObject* object : m_sceneManager->objects()) {
        const QString marker = object->id() == m_selectedObjectId ? "[Selected]" : "";
        const QString visibility = object->isVisible() ? "Visible" : "Hidden";
        painter.drawText(12, y, QString("#%1 %2 %3 (%4)")
                                    .arg(object->id())
                                    .arg(object->name())
                                    .arg(marker)
                                    .arg(visibility));
        y += 18;
    }
}
