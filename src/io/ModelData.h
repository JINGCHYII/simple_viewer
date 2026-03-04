#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

struct vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec3 color;
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct PointCloudData
{
    std::vector<Vertex> points;
};

inline vec3 operator+(const vec3 &a, const vec3 &b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline vec3 operator-(const vec3 &a, const vec3 &b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline vec3 operator*(const vec3 &v, float s)
{
    return {v.x * s, v.y * s, v.z * s};
}

inline vec3 cross(const vec3 &a, const vec3 &b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

inline float length(const vec3 &v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline vec3 normalize(const vec3 &v)
{
    const float len = length(v);
    if (len <= 1e-8f) {
        return {0.0f, 0.0f, 0.0f};
    }

    return v * (1.0f / len);
}
