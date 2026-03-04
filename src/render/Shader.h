#pragma once

#include <QMatrix4x4>
#include <QOpenGLFunctions_3_3_Core>
#include <QVector3D>

class Shader
{
public:
    bool load(QOpenGLFunctions_3_3_Core *gl, const char *vertexPath, const char *fragmentPath);
    void bind(QOpenGLFunctions_3_3_Core *gl) const;
    void release(QOpenGLFunctions_3_3_Core *gl) const;
    void clear(QOpenGLFunctions_3_3_Core *gl);

    bool isValid() const;

    void setMat4(QOpenGLFunctions_3_3_Core *gl, const char *name, const QMatrix4x4 &value) const;
    void setVec3(QOpenGLFunctions_3_3_Core *gl, const char *name, const QVector3D &value) const;
    void setFloat(QOpenGLFunctions_3_3_Core *gl, const char *name, float value) const;
    void setInt(QOpenGLFunctions_3_3_Core *gl, const char *name, int value) const;

private:
    unsigned int compile(QOpenGLFunctions_3_3_Core *gl, unsigned int type, const QByteArray &source) const;
    int uniformLocation(QOpenGLFunctions_3_3_Core *gl, const char *name) const;

    unsigned int m_program = 0;
};
