#pragma once

#include "io/ModelData.h"

#include <QString>

class STLLoader
{
public:
    bool load(const QString &path, MeshData &meshData, bool deduplicate = false) const;

private:
    bool loadAscii(const QString &path, MeshData &meshData) const;
    bool loadBinary(const QString &path, MeshData &meshData) const;
};
