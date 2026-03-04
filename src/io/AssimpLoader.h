#pragma once

#include "io/ModelData.h"

#include <QString>

class AssimpLoader
{
public:
    bool load(const QString &path,
              MeshData &meshData,
              PointCloudData &pointCloudData,
              bool &isMesh,
              bool joinIdenticalVertices = true) const;
};
