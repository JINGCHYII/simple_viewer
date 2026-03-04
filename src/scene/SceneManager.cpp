#include "SceneManager.h"

SceneManager::SceneManager(QObject* parent)
    : QObject(parent)
{
}

SceneObject* SceneManager::addObject(const QString& name, const QString& sourcePath)
{
    SceneObject* object = new SceneObject(m_nextId++, name, sourcePath);
    m_objects.push_back(object);
    emit objectAdded(object);
    return object;
}

bool SceneManager::removeObject(int id)
{
    for (int i = 0; i < m_objects.size(); ++i) {
        if (m_objects[i]->id() == id) {
            delete m_objects[i];
            m_objects.removeAt(i);
            emit objectRemoved(id);
            return true;
        }
    }
    return false;
}

QVector<SceneObject*>& SceneManager::objects()
{
    return m_objects;
}

const QVector<SceneObject*>& SceneManager::objects() const
{
    return m_objects;
}

SceneObject* SceneManager::objectById(int id) const
{
    for (SceneObject* object : m_objects) {
        if (object->id() == id) {
            return object;
        }
    }
    return nullptr;
}
