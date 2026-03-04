#version 330 core

in VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 viewPos;
    vec3 viewNormal;
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

uniform int uShadingModel;
uniform float uRimStrength;
uniform float uFlatFactor;
uniform float uGamma;

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

    float hemi = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 hemiAmbient = mix(uAmbientColor * vec3(0.38, 0.38, 0.40), uAmbientColor, hemi);
    vec3 ambient = uAmbientStrength * hemiAmbient * baseColor;
    vec3 diffuse = diff * uLightColor * baseColor;
    vec3 specular = uSpecularStrength * spec * uLightColor;

    vec3 color;
    if (uShadingModel == 1) {
        float nl = max(dot(n, lightDir), 0.0);
        float quantized = floor(nl * 4.0) / 4.0;
        float mixedDiffuse = mix(nl, quantized, clamp(uFlatFactor, 0.0, 1.0));

        vec3 viewN = normalize(fs_in.viewNormal);
        vec3 viewV = normalize(-fs_in.viewPos);
        float rim = pow(clamp(1.0 - max(dot(viewN, viewV), 0.0), 0.0, 1.0), 1.5);

        vec3 rimColor = baseColor * (0.25 + 0.75 * uLightColor);
        color = ambient + mixedDiffuse * uLightColor * baseColor + specular + rim * uRimStrength * rimColor;
    } else {
        color = ambient + diffuse + specular;
    }

    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);
    color = clamp(color, 0.0, 1.0);
    color = pow(color, vec3(1.0 / max(uGamma, 0.001)));

    FragColor = vec4(color, 1.0);
}
