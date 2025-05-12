#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// Inputs from vertex shader
layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) flat in uint vMaterial;
layout(location = 4) in vec4 vTangent;

// Texture array for base color
layout(binding = 1) uniform sampler2D textures[];

// G-Buffer outputs
layout(location = 0) out vec4 outAlbedo;   // Albedo (RGBA, VK_FORMAT_R8G8B8A8_UNORM)
layout(location = 1) out vec2 outNormal;   // Normal.xy (RG16F, VK_FORMAT_R16G16_SFLOAT)
layout(location = 2) out vec2 outParams;   // Roughness/Metallic (RG8, VK_FORMAT_R8G8_UNORM)

void main() {
    // Albedo (base color)
    outAlbedo = texture(textures[vMaterial], vTexCoord);

    // Normal: encode world-space normal.xy into [0,1] for RG16F
    vec3 N = normalize(vNormal);
    outNormal = N.xy * 0.5 + 0.5;

    // Params: placeholder roughness/metallic
    float roughness = 1.0;
    float metallic = 0.0;
    outParams = vec2(roughness, metallic);
}