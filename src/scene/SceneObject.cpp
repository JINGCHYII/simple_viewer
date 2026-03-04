#include "SceneObject.h"

SceneObject::SceneObject(int id, QString name, QString sourcePath)
    : m_id(id), m_name(std::move(name)), m_sourcePath(std::move(sourcePath))
{
}

int SceneObject::id() const
{
    return m_id;
}

const QString& SceneObject::name() const
{
    return m_name;
}

const QString& SceneObject::sourcePath() const
{
    return m_sourcePath;
}

void SceneObject::setName(const QString& name)
{
    m_name = name;
}

bool SceneObject::isVisible() const
{
    return m_visible;
}

void SceneObject::setVisible(bool visible)
{
    m_visible = visible;
}

Transform& SceneObject::transform()
{
    return m_transform;
}

const Transform& SceneObject::transform() const
{
    return m_transform;
}
