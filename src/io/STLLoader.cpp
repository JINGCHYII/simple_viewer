#include "io/STLLoader.h"

#include <QDataStream>
#include <QFile>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>

#include <unordered_map>
#include <utility>

namespace {

struct VertexKey
{
    int px;
    int py;
    int pz;
    int nx;
    int ny;
    int nz;
    int cx;
    int cy;
    int cz;

    bool operator==(const VertexKey &other) const
    {
        return px == other.px && py == other.py && pz == other.pz &&
               nx == other.nx && ny == other.ny && nz == other.nz &&
               cx == other.cx && cy == other.cy && cz == other.cz;
    }
};

struct VertexKeyHasher
{
    std::size_t operator()(const VertexKey &key) const
    {
        std::size_t seed = 0;
        auto combine = [&seed](int value) {
            seed ^= std::hash<int>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        combine(key.px); combine(key.py); combine(key.pz);
        combine(key.nx); combine(key.ny); combine(key.nz);
        combine(key.cx); combine(key.cy); combine(key.cz);
        return seed;
    }
};

VertexKey toVertexKey(const Vertex &vertex)
{
    constexpr float scale = 1000000.0f;
    return {
        static_cast<int>(vertex.pos.x * scale),
        static_cast<int>(vertex.pos.y * scale),
        static_cast<int>(vertex.pos.z * scale),
        static_cast<int>(vertex.normal.x * scale),
        static_cast<int>(vertex.normal.y * scale),
        static_cast<int>(vertex.normal.z * scale),
        static_cast<int>(vertex.color.x * scale),
        static_cast<int>(vertex.color.y * scale),
        static_cast<int>(vertex.color.z * scale),
    };
}

bool probablyAscii(const QByteArray &raw)
{
    if (raw.size() < 6) {
        return false;
    }

    return raw.startsWith("solid");
}

}

bool STLLoader::load(const QString &path, MeshData &meshData, bool deduplicate) const
{
    meshData = {};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray head = file.peek(256);
    file.close();

    bool loaded = false;
    if (probablyAscii(head)) {
        loaded = loadAscii(path, meshData);
        if (!loaded) {
            loaded = loadBinary(path, meshData);
        }
    } else {
        loaded = loadBinary(path, meshData);
        if (!loaded) {
            loaded = loadAscii(path, meshData);
        }
    }

    if (!loaded) {
        return false;
    }

    if (!deduplicate) {
        return true;
    }

    MeshData deduped;
    deduped.indices.reserve(meshData.indices.size());
    std::unordered_map<VertexKey, std::uint32_t, VertexKeyHasher> map;

    for (std::uint32_t index : meshData.indices) {
        if (index >= meshData.vertices.size()) {
            continue;
        }

        const Vertex &vertex = meshData.vertices[index];
        const VertexKey key = toVertexKey(vertex);
        auto it = map.find(key);
        if (it == map.end()) {
            const std::uint32_t newIndex = static_cast<std::uint32_t>(deduped.vertices.size());
            deduped.vertices.push_back(vertex);
            deduped.indices.push_back(newIndex);
            map[key] = newIndex;
        } else {
            deduped.indices.push_back(it->second);
        }
    }

    meshData = std::move(deduped);
    return true;
}

bool STLLoader::loadAscii(const QString &path, MeshData &meshData) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    vec3 currentNormal = {0.0f, 0.0f, 1.0f};
    std::vector<vec3> faceVertices;

    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        const QStringList tokens = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if (tokens.isEmpty()) {
            continue;
        }

        if (tokens[0] == "facet" && tokens.size() >= 5 && tokens[1] == "normal") {
            currentNormal = {
                tokens[2].toFloat(),
                tokens[3].toFloat(),
                tokens[4].toFloat(),
            };
            faceVertices.clear();
            continue;
        }

        if (tokens[0] == "vertex" && tokens.size() >= 4) {
            faceVertices.push_back({
                tokens[1].toFloat(),
                tokens[2].toFloat(),
                tokens[3].toFloat(),
            });
            continue;
        }

        if (tokens[0] == "endfacet") {
            if (faceVertices.size() >= 3) {
                const vec3 normal = normalize(currentNormal);
                for (int i = 0; i < 3; ++i) {
                    Vertex vertex;
                    vertex.pos = faceVertices[i];
                    vertex.normal = normal;
                    vertex.color = {0.8f, 0.8f, 0.8f};
                    meshData.vertices.push_back(vertex);
                    meshData.indices.push_back(static_cast<std::uint32_t>(meshData.vertices.size() - 1));
                }
            }
            faceVertices.clear();
        }
    }

    return !meshData.indices.empty();
}

bool STLLoader::loadBinary(const QString &path, MeshData &meshData) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    char header[80];
    if (stream.readRawData(header, 80) != 80) {
        return false;
    }

    std::uint32_t triangleCount = 0;
    stream >> triangleCount;
    if (triangleCount == 0) {
        return false;
    }

    meshData.vertices.reserve(static_cast<std::size_t>(triangleCount) * 3);
    meshData.indices.reserve(static_cast<std::size_t>(triangleCount) * 3);

    for (std::uint32_t t = 0; t < triangleCount; ++t) {
        float nx = 0.0f;
        float ny = 0.0f;
        float nz = 1.0f;
        stream >> nx >> ny >> nz;
        const vec3 normal = normalize({nx, ny, nz});

        for (int v = 0; v < 3; ++v) {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            stream >> x >> y >> z;

            Vertex vertex;
            vertex.pos = {x, y, z};
            vertex.normal = normal;
            vertex.color = {0.8f, 0.8f, 0.8f};
            meshData.vertices.push_back(vertex);
            meshData.indices.push_back(static_cast<std::uint32_t>(meshData.vertices.size() - 1));
        }

        std::uint16_t attributes = 0;
        stream >> attributes;
    }

    return !meshData.indices.empty();
}
