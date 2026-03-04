#pragma once

#include "io/ModelData.h"

#include <QOpenGLFunctions_3_3_Core>

class Mesh
{
public:
    void upload(const MeshData &data);
    void draw(QOpenGLFunctions_3_3_Core *gl) const;
    void clear();

    bool isValid() const;

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    int m_indexCount = 0;
};
