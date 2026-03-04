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

    vec3 ambient = uAmbientStrength * uAmbientColor * baseColor;
    vec3 diffuse = diff * uLightColor * baseColor;
    vec3 specular = uSpecularStrength * spec * uLightColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
