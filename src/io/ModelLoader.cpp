#include "io/ModelLoader.h"

#include "io/AssimpLoader.h"

bool ModelLoader::load(const QString &path)
{
    m_meshData = {};
    m_pointCloudData = {};
    m_isMesh = false;
    m_isPointCloud = false;

    if (path.isEmpty()) {
        return false;
    }

    AssimpLoader assimpLoader;
    if (!assimpLoader.load(path, m_meshData, m_pointCloudData, m_isMesh, true)) {
        return false;
    }

    m_isPointCloud = !m_pointCloudData.points.empty();
    if (m_isMesh) {
        rebuildMeshNormals();
    }

    return true;
}

const MeshData &ModelLoader::meshData() const
{
    return m_meshData;
}

const PointCloudData &ModelLoader::pointCloudData() const
{
    return m_pointCloudData;
}

bool ModelLoader::hasMesh() const
{
    return m_isMesh;
}

bool ModelLoader::hasPointCloud() const
{
    return m_isPointCloud;
}

void ModelLoader::rebuildMeshNormals()
{
    if (m_meshData.vertices.empty() || m_meshData.indices.empty()) {
        return;
    }

    bool hasValidNormals = false;
    for (const Vertex &vertex : m_meshData.vertices) {
        if (length(vertex.normal) > 1e-6f) {
            hasValidNormals = true;
            break;
        }
    }

    if (hasValidNormals) {
        return;
    }

    for (Vertex &vertex : m_meshData.vertices) {
        vertex.normal = {0.0f, 0.0f, 0.0f};
    }

    for (std::size_t i = 0; i + 2 < m_meshData.indices.size(); i += 3) {
        const std::uint32_t i0 = m_meshData.indices[i];
        const std::uint32_t i1 = m_meshData.indices[i + 1];
        const std::uint32_t i2 = m_meshData.indices[i + 2];

        if (i0 >= m_meshData.vertices.size() || i1 >= m_meshData.vertices.size() || i2 >= m_meshData.vertices.size()) {
            continue;
        }

        const vec3 &p0 = m_meshData.vertices[i0].pos;
        const vec3 &p1 = m_meshData.vertices[i1].pos;
        const vec3 &p2 = m_meshData.vertices[i2].pos;

        const vec3 faceNormal = normalize(cross(p1 - p0, p2 - p0));
        m_meshData.vertices[i0].normal = m_meshData.vertices[i0].normal + faceNormal;
        m_meshData.vertices[i1].normal = m_meshData.vertices[i1].normal + faceNormal;
        m_meshData.vertices[i2].normal = m_meshData.vertices[i2].normal + faceNormal;
    }

    for (Vertex &vertex : m_meshData.vertices) {
        vertex.normal = normalize(vertex.normal);
    }
}
