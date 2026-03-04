#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 color;
} vs_out;

void main()
{
    vec4 world = uModel * vec4(aPos, 1.0);
    vs_out.worldPos = world.xyz;
    vs_out.normal = mat3(transpose(inverse(uModel))) * aNormal;
    vs_out.color = aColor;
    gl_Position = uProj * uView * world;
}
