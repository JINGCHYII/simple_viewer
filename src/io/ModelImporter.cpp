#include "ModelImporter.h"

#include <QFileInfo>

ImportResult ModelImporter::importModel(const QString& path)
{
    QFileInfo file(path);
    if (!file.exists() || !file.isFile()) {
        return {false, "File does not exist.", {}};
    }

    const QString suffix = file.suffix().toLower();
    if (suffix != "ply" && suffix != "stl" && suffix != "obj") {
        return {false, "Unsupported format. Supported: .ply, .stl, .obj", {}};
    }

    return {true, "Import succeeded.", file.completeBaseName()};
}
