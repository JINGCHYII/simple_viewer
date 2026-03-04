#include "io/PLYLoader.h"

#include <QFile>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>

namespace {

struct PLYProperty
{
    QString name;
};

float readAsFloat(const QString &token)
{
    return token.toFloat();
}

}

bool PLYLoader::load(const QString &path, MeshData &meshData, PointCloudData &pointCloudData, bool &isMesh) const
{
    meshData = {};
    pointCloudData = {};
    isMesh = false;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);

    if (stream.readLine().trimmed() != "ply") {
        return false;
    }

    int vertexCount = 0;
    int faceCount = 0;
    bool asciiFormat = false;
    bool inVertexElement = false;
    QList<PLYProperty> vertexProperties;

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        if (line == "end_header") {
            break;
        }

        const QStringList tokens = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if (tokens.isEmpty()) {
            continue;
        }

        if (tokens[0] == "format" && tokens.size() >= 2) {
            asciiFormat = (tokens[1] == "ascii");
            continue;
        }

        if (tokens[0] == "element" && tokens.size() >= 3) {
            inVertexElement = (tokens[1] == "vertex");
            if (tokens[1] == "vertex") {
                vertexCount = tokens[2].toInt();
            } else if (tokens[1] == "face") {
                faceCount = tokens[2].toInt();
            }
            continue;
        }

        if (tokens[0] == "property" && inVertexElement) {
            if (tokens.size() >= 3 && tokens[1] != "list") {
                vertexProperties.push_back({tokens[2]});
            }
            continue;
        }
    }

    if (!asciiFormat || vertexCount <= 0) {
        return false;
    }

    meshData.vertices.reserve(vertexCount);
    pointCloudData.points.reserve(vertexCount);

    int posXIndex = -1;
    int posYIndex = -1;
    int posZIndex = -1;
    int nxIndex = -1;
    int nyIndex = -1;
    int nzIndex = -1;
    int rIndex = -1;
    int gIndex = -1;
    int bIndex = -1;

    for (int i = 0; i < vertexProperties.size(); ++i) {
        const QString name = vertexProperties[i].name;
        if (name == "x") posXIndex = i;
        if (name == "y") posYIndex = i;
        if (name == "z") posZIndex = i;
        if (name == "nx" || name == "normal_x") nxIndex = i;
        if (name == "ny" || name == "normal_y") nyIndex = i;
        if (name == "nz" || name == "normal_z") nzIndex = i;
        if (name == "red" || name == "r") rIndex = i;
        if (name == "green" || name == "g") gIndex = i;
        if (name == "blue" || name == "b") bIndex = i;
    }

    for (int v = 0; v < vertexCount && !stream.atEnd(); ++v) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty()) {
            --v;
            continue;
        }

        const QStringList tokens = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if (tokens.size() < vertexProperties.size()) {
            return false;
        }

        Vertex vertex;
        vertex.color = {1.0f, 1.0f, 1.0f};

        if (posXIndex >= 0) vertex.pos.x = readAsFloat(tokens[posXIndex]);
        if (posYIndex >= 0) vertex.pos.y = readAsFloat(tokens[posYIndex]);
        if (posZIndex >= 0) vertex.pos.z = readAsFloat(tokens[posZIndex]);

        if (nxIndex >= 0) vertex.normal.x = readAsFloat(tokens[nxIndex]);
        if (nyIndex >= 0) vertex.normal.y = readAsFloat(tokens[nyIndex]);
        if (nzIndex >= 0) vertex.normal.z = readAsFloat(tokens[nzIndex]);

        if (rIndex >= 0) vertex.color.x = readAsFloat(tokens[rIndex]) / 255.0f;
        if (gIndex >= 0) vertex.color.y = readAsFloat(tokens[gIndex]) / 255.0f;
        if (bIndex >= 0) vertex.color.z = readAsFloat(tokens[bIndex]) / 255.0f;

        meshData.vertices.push_back(vertex);
        pointCloudData.points.push_back(vertex);
    }

    if (faceCount <= 0) {
        isMesh = false;
        return true;
    }

    meshData.indices.reserve(faceCount * 3);

    for (int f = 0; f < faceCount && !stream.atEnd(); ++f) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty()) {
            --f;
            continue;
        }

        const QStringList tokens = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if (tokens.isEmpty()) {
            continue;
        }

        const int n = tokens[0].toInt();
        if (tokens.size() < n + 1 || n < 3) {
            continue;
        }

        std::vector<std::uint32_t> faceIndices;
        faceIndices.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            faceIndices.push_back(static_cast<std::uint32_t>(tokens[i + 1].toUInt()));
        }

        for (int i = 1; i + 1 < n; ++i) {
            meshData.indices.push_back(faceIndices[0]);
            meshData.indices.push_back(faceIndices[i]);
            meshData.indices.push_back(faceIndices[i + 1]);
        }
    }

    isMesh = !meshData.indices.empty();
    if (!isMesh) {
        meshData = {};
    }

    return true;
}
