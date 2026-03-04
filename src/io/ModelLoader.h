#pragma once

#include "io/ModelData.h"

#include <QString>

class ModelLoader
{
public:
    bool load(const QString &path);

    const MeshData &meshData() const;
    const PointCloudData &pointCloudData() const;

    bool hasMesh() const;
    bool hasPointCloud() const;

private:
    void rebuildMeshNormals();

    MeshData m_meshData;
    PointCloudData m_pointCloudData;
    bool m_isMesh = false;
    bool m_isPointCloud = false;
};
