#include "render/Shader.h"

#include <QFile>
#include <QTextStream>

namespace
{
QByteArray loadTextFile(const char *path)
{
    QFile file(QString::fromUtf8(path));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    return file.readAll();
}
}

bool Shader::load(QOpenGLFunctions_3_3_Core *gl, const char *vertexPath, const char *fragmentPath)
{
    if (gl == nullptr) {
        return false;
    }

    clear(gl);

    const QByteArray vertSource = loadTextFile(vertexPath);
    const QByteArray fragSource = loadTextFile(fragmentPath);
    if (vertSource.isEmpty() || fragSource.isEmpty()) {
        return false;
    }

    const unsigned int vertexShader = compile(gl, GL_VERTEX_SHADER, vertSource);
    const unsigned int fragmentShader = compile(gl, GL_FRAGMENT_SHADER, fragSource);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) {
            gl->glDeleteShader(vertexShader);
        }
        if (fragmentShader != 0) {
            gl->glDeleteShader(fragmentShader);
        }
        return false;
    }

    m_program = gl->glCreateProgram();
    gl->glAttachShader(m_program, vertexShader);
    gl->glAttachShader(m_program, fragmentShader);
    gl->glLinkProgram(m_program);

    int linkStatus = 0;
    gl->glGetProgramiv(m_program, GL_LINK_STATUS, &linkStatus);

    gl->glDeleteShader(vertexShader);
    gl->glDeleteShader(fragmentShader);

    if (linkStatus != GL_TRUE) {
        clear(gl);
        return false;
    }

    return true;
}

void Shader::bind(QOpenGLFunctions_3_3_Core *gl) const
{
    if (gl != nullptr && m_program != 0) {
        gl->glUseProgram(m_program);
    }
}

void Shader::release(QOpenGLFunctions_3_3_Core *gl) const
{
    if (gl != nullptr) {
        gl->glUseProgram(0);
    }
}

void Shader::clear(QOpenGLFunctions_3_3_Core *gl)
{
    if (gl != nullptr && m_program != 0) {
        gl->glDeleteProgram(m_program);
    }
    m_program = 0;
}

bool Shader::isValid() const
{
    return m_program != 0;
}

void Shader::setMat4(QOpenGLFunctions_3_3_Core *gl, const char *name, const QMatrix4x4 &value) const
{
    const int loc = uniformLocation(gl, name);
    if (loc >= 0) {
        gl->glUniformMatrix4fv(loc, 1, GL_FALSE, value.constData());
    }
}

void Shader::setVec3(QOpenGLFunctions_3_3_Core *gl, const char *name, const QVector3D &value) const
{
    const int loc = uniformLocation(gl, name);
    if (loc >= 0) {
        gl->glUniform3f(loc, value.x(), value.y(), value.z());
    }
}

void Shader::setFloat(QOpenGLFunctions_3_3_Core *gl, const char *name, float value) const
{
    const int loc = uniformLocation(gl, name);
    if (loc >= 0) {
        gl->glUniform1f(loc, value);
    }
}

void Shader::setInt(QOpenGLFunctions_3_3_Core *gl, const char *name, int value) const
{
    const int loc = uniformLocation(gl, name);
    if (loc >= 0) {
        gl->glUniform1i(loc, value);
    }
}

unsigned int Shader::compile(QOpenGLFunctions_3_3_Core *gl, unsigned int type, const QByteArray &source) const
{
    const unsigned int shader = gl->glCreateShader(type);
    const char *sourcePtr = source.constData();
    gl->glShaderSource(shader, 1, &sourcePtr, nullptr);
    gl->glCompileShader(shader);

    int compileStatus = 0;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus != GL_TRUE) {
        gl->glDeleteShader(shader);
        return 0;
    }

    return shader;
}

int Shader::uniformLocation(QOpenGLFunctions_3_3_Core *gl, const char *name) const
{
    if (gl == nullptr || m_program == 0 || name == nullptr) {
        return -1;
    }
    return gl->glGetUniformLocation(m_program, name);
}
