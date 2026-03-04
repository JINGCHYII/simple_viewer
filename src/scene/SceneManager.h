#pragma once

#include "SceneObject.h"

#include <QObject>
#include <QVector>

class SceneManager : public QObject {
    Q_OBJECT

public:
    explicit SceneManager(QObject* parent = nullptr);

    SceneObject* addObject(const QString& name, const QString& sourcePath);
    bool removeObject(int id);

    QVector<SceneObject*>& objects();
    const QVector<SceneObject*>& objects() const;

    SceneObject* objectById(int id) const;

signals:
    void objectAdded(SceneObject* object);
    void objectRemoved(int id);
    void objectUpdated(int id);

private:
    QVector<SceneObject*> m_objects;
    int m_nextId {1};
};
