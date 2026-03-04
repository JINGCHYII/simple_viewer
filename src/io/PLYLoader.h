#pragma once

#include "io/ModelData.h"

#include <QString>

class PLYLoader
{
public:
    bool load(const QString &path, MeshData &meshData, PointCloudData &pointCloudData, bool &isMesh) const;
};
