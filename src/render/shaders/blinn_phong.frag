#version 330 core

in VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 color;
} fs_in;

uniform vec3 uLightPos;
uniform vec3 uViewPos;

uniform vec3 uAmbientColor;
uniform vec3 uLightColor;
uniform vec3 uMaterialColor;
uniform float uAmbientStrength;
uniform float uSpecularStrength;
uniform float uShininess;
uniform int uUseVertexColor;
uniform int uEnableSpecular;
uniform int uPointColorMode;
uniform vec3 uBoundsMin;
uniform vec3 uBoundsMax;

out vec4 FragColor;

void main()
{
    vec3 n = normalize(fs_in.normal);
    vec3 lightDir = normalize(uLightPos - fs_in.worldPos);
    vec3 viewDir = normalize(uViewPos - fs_in.worldPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    float diff = max(dot(n, lightDir), 0.0);

    float spec = 0.0;
    if (uEnableSpecular != 0 && diff > 0.0) {
        spec = pow(max(dot(n, halfDir), 0.0), uShininess);
    }

    vec3 baseColor = (uUseVertexColor != 0) ? fs_in.color : uMaterialColor;
    if (uPointColorMode == 1) {
        float height = (fs_in.worldPos.y - uBoundsMin.y) / max(uBoundsMax.y - uBoundsMin.y, 1e-5);
        height = clamp(height, 0.0, 1.0);
        baseColor = mix(vec3(0.2, 0.45, 0.95), vec3(0.95, 0.35, 0.2), height);
    }

    vec3 ambient = uAmbientStrength * uAmbientColor * baseColor;
    vec3 diffuse = diff * uLightColor * baseColor;
    vec3 specular = uSpecularStrength * spec * uLightColor;

    vec3 color = ambient + diffuse + specular;
    color = color / (vec3(1.0) + color);
    color = clamp(color, 0.0, 1.0);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
