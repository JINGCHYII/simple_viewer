#include "io/AssimpLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstddef>

namespace {

vec3 toVec3(const aiVector3D &v)
{
    return {v.x, v.y, v.z};
}

vec3 toColor(const aiColor4D &c)
{
    return {c.r, c.g, c.b};
}

}

bool AssimpLoader::load(const QString &path,
                        MeshData &meshData,
                        PointCloudData &pointCloudData,
                        bool &isMesh,
                        bool joinIdenticalVertices) const
{
    meshData = {};
    pointCloudData = {};
    isMesh = false;

    if (path.isEmpty()) {
        return false;
    }

    Assimp::Importer importer;
    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals;
    if (joinIdenticalVertices) {
        flags |= aiProcess_JoinIdenticalVertices;
    }

    const aiScene *scene = importer.ReadFile(path.toStdString(), flags);
    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mNumMeshes == 0) {
        return false;
    }

    std::size_t totalVertices = 0;
    std::size_t totalIndices = 0;
    bool hasAnyFace = false;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh *mesh = scene->mMeshes[m];
        if (mesh == nullptr) {
            continue;
        }

        totalVertices += mesh->mNumVertices;
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace &face = mesh->mFaces[f];
            totalIndices += face.mNumIndices;
            if (face.mNumIndices >= 3) {
                hasAnyFace = true;
            }
        }
    }

    meshData.vertices.reserve(totalVertices);
    meshData.indices.reserve(totalIndices);
    pointCloudData.points.reserve(totalVertices);

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh *mesh = scene->mMeshes[m];
        if (mesh == nullptr) {
            continue;
        }

        const std::uint32_t vertexOffset = static_cast<std::uint32_t>(meshData.vertices.size());
        const bool hasNormals = mesh->HasNormals();
        const bool hasColors = mesh->HasVertexColors(0);

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            Vertex vertex;
            vertex.pos = toVec3(mesh->mVertices[v]);
            vertex.normal = hasNormals ? toVec3(mesh->mNormals[v]) : vec3{0.0f, 0.0f, 0.0f};
            vertex.color = hasColors ? toColor(mesh->mColors[0][v]) : vec3{1.0f, 1.0f, 1.0f};

            meshData.vertices.push_back(vertex);
            pointCloudData.points.push_back(vertex);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace &face = mesh->mFaces[f];
            for (unsigned int i = 0; i < face.mNumIndices; ++i) {
                meshData.indices.push_back(vertexOffset + face.mIndices[i]);
            }
        }
    }

    isMesh = hasAnyFace && !meshData.indices.empty();
    if (!isMesh) {
        meshData = {};
    }

    return !pointCloudData.points.empty();
}
