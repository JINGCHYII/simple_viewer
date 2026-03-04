#include "render/PointCloud.h"

#include <QOpenGLContext>

#include <cstddef>

void PointCloud::upload(const PointCloudData &data)
{
    QOpenGLFunctions_3_3_Core *gl = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
    if (gl == nullptr) {
        return;
    }

    clear();

    if (data.points.empty()) {
        return;
    }

    gl->glGenVertexArrays(1, &m_vao);
    gl->glGenBuffers(1, &m_vbo);

    gl->glBindVertexArray(m_vao);
    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    gl->glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(data.points.size() * sizeof(Vertex)),
                     data.points.data(),
                     GL_STATIC_DRAW);

    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, pos)));

    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));

    gl->glEnableVertexAttribArray(2);
    gl->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, color)));

    gl->glBindVertexArray(0);

    m_pointCount = static_cast<int>(data.points.size());
}

void PointCloud::draw(QOpenGLFunctions_3_3_Core *gl) const
{
    if (gl == nullptr || m_vao == 0 || m_pointCount <= 0) {
        return;
    }

    gl->glBindVertexArray(m_vao);
    gl->glDrawArrays(GL_POINTS, 0, m_pointCount);
    gl->glBindVertexArray(0);
}

void PointCloud::clear()
{
    QOpenGLFunctions_3_3_Core *gl = QOpenGLContext::currentContext() != nullptr
                                        ? QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>()
                                        : nullptr;

    if (gl != nullptr) {
        if (m_vbo != 0) {
            gl->glDeleteBuffers(1, &m_vbo);
        }
        if (m_vao != 0) {
            gl->glDeleteVertexArrays(1, &m_vao);
        }
    }

    m_vao = 0;
    m_vbo = 0;
    m_pointCount = 0;
}

bool PointCloud::isValid() const
{
    return m_vao != 0 && m_pointCount > 0;
}
