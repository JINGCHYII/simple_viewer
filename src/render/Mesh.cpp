#include "render/Mesh.h"

#include <QOpenGLContext>

#include <cstddef>

void Mesh::upload(const MeshData &data)
{
    QOpenGLFunctions_3_3_Core *gl = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
    if (gl == nullptr) {
        return;
    }

    clear();

    if (data.vertices.empty() || data.indices.empty()) {
        return;
    }

    gl->glGenVertexArrays(1, &m_vao);
    gl->glGenBuffers(1, &m_vbo);
    gl->glGenBuffers(1, &m_ebo);

    gl->glBindVertexArray(m_vao);

    gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    gl->glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(data.vertices.size() * sizeof(Vertex)),
                     data.vertices.data(),
                     GL_STATIC_DRAW);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(data.indices.size() * sizeof(std::uint32_t)),
                     data.indices.data(),
                     GL_STATIC_DRAW);

    gl->glEnableVertexAttribArray(0);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, pos)));

    gl->glEnableVertexAttribArray(1);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));

    gl->glEnableVertexAttribArray(2);
    gl->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, color)));

    gl->glBindVertexArray(0);

    m_indexCount = static_cast<int>(data.indices.size());
}

void Mesh::draw(QOpenGLFunctions_3_3_Core *gl) const
{
    if (gl == nullptr || m_vao == 0 || m_indexCount <= 0) {
        return;
    }

    gl->glBindVertexArray(m_vao);
    gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    gl->glBindVertexArray(0);
}

void Mesh::clear()
{
    QOpenGLFunctions_3_3_Core *gl = QOpenGLContext::currentContext() != nullptr
                                        ? QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>()
                                        : nullptr;

    if (gl != nullptr) {
        if (m_ebo != 0) {
            gl->glDeleteBuffers(1, &m_ebo);
        }
        if (m_vbo != 0) {
            gl->glDeleteBuffers(1, &m_vbo);
        }
        if (m_vao != 0) {
            gl->glDeleteVertexArrays(1, &m_vao);
        }
    }

    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_indexCount = 0;
}

bool Mesh::isValid() const
{
    return m_vao != 0 && m_indexCount > 0;
}
