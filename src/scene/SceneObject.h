#pragma once

#include <QMatrix4x4>
#include <QString>
#include <QVector3D>

struct Transform {
    QVector3D translation {0.0f, 0.0f, 0.0f};
    QVector3D rotationEuler {0.0f, 0.0f, 0.0f};
    QVector3D scale {1.0f, 1.0f, 1.0f};
};

class SceneObject {
public:
    SceneObject(int id, QString name, QString sourcePath);

    int id() const;
    const QString& name() const;
    const QString& sourcePath() const;

    void setName(const QString& name);

    bool isVisible() const;
    void setVisible(bool visible);

    Transform& transform();
    const Transform& transform() const;

private:
    int m_id;
    QString m_name;
    QString m_sourcePath;
    bool m_visible {true};
    Transform m_transform;
};
