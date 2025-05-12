#version 450

layout(binding = 1) uniform sampler2D gAlbedo;  // RGBA, VK_FORMAT_R8G8B8A8_UNORM
layout(binding = 2) uniform sampler2D gNormal;  // RG16F, VK_FORMAT_R16G16_SFLOAT
layout(binding = 3) uniform sampler2D gParams;  // RG8, VK_FORMAT_R8G8_UNORM

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    // Sample G-buffer textures
    vec3 albedo = texture(gAlbedo, fragUV).rgb;
    vec2 encodedNormal = texture(gNormal, fragUV).rg;
    vec2 params = texture(gParams, fragUV).rg;
    float roughness = params.x;
    float metallic = params.y;

    vec2 decodedNormalXY = encodedNormal * 2.0 - 1.0;
    float z = sqrt(1.0 - dot(decodedNormalXY, decodedNormalXY)); // Reconstruct z
    vec3 normal = normalize(vec3(decodedNormalXY, z));

    vec3 lightDir = normalize(vec3(-0.5, 1.0, 0.3));
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 color = albedo * diff * (1.0 - roughness * 0.5);

    outColor = vec4(color, 1.0);
}