#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform int uIsPointRender;
uniform float uPointSize;

out VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 viewPos;
    vec3 viewNormal;
    vec3 color;
} vs_out;

void main()
{
    vec4 world = uModel * vec4(aPos, 1.0);
    mat3 normalMat = mat3(transpose(inverse(uModel)));

    vs_out.worldPos = world.xyz;
    vs_out.normal = normalMat * aNormal;

    vec4 viewWorld = uView * world;
    vs_out.viewPos = viewWorld.xyz;
    vs_out.viewNormal = mat3(uView) * vs_out.normal;

    vs_out.color = aColor;
    gl_Position = uProj * viewWorld;
    gl_PointSize = (uIsPointRender != 0) ? uPointSize : 1.0;
}
