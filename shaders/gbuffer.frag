#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// Inputs from vertex shader
layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) flat in uint vBaseColorTexture;
layout(location = 4) in vec4 vTangent;
layout(location = 5) flat in uint vMetalRoughTextureIndex;

// Texture array for base color
layout(binding = 1) uniform sampler2D textures[];
layout(binding = 5) uniform sampler2D metallicRoughnessTextures[];

// G-Buffer outputs
layout(location = 0) out vec4 outAlbedo;   // Albedo (RGBA, VK_FORMAT_R8G8B8A8_UNORM)
layout(location = 1) out vec2 outNormal;   // Normal.xy (RG16F, VK_FORMAT_R16G16_SFLOAT)
layout(location = 2) out vec2 outParams;   // Roughness/Metallic (RG8, VK_FORMAT_R8G8_UNORM)

void main() {
    vec4 albedo = texture(textures[vBaseColorTexture], vTexCoord);
    // Match depth pass's alpha cutoff
    if (albedo.a < 0.95) discard;
    outAlbedo = albedo;

    // Encode normals (world space)
    vec3 N = normalize(vNormal);
    outNormal.xy = N.xy * 0.5 + 0.5;

    vec4 mrSample = texture(metallicRoughnessTextures[vMetalRoughTextureIndex], vTexCoord);
    float roughness = mrSample.g;
    float metallic = mrSample.b;

    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    // Write PBR parameters
    outParams = vec2(roughness, metallic);
}
