#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// Inputs from vertex shader
layout(location = 0) in vec3 vWorldPos;     // For depth if needed or lighting
layout(location = 1) in vec3 vNormal;       // Full 3D normal
layout(location = 2) in vec2 vTexCoord;     // UV
layout(location = 3) in vec4 vTangent;      // Tangent + bitangent sign
layout(location = 4) flat in uint baseColorTexture; // Material/texture index
layout(location = 5) flat in uint metalRoughTextureIndex;
layout(location = 6) flat in uint normalTextureIndex;
layout(location = 7) flat in uint textureCount;

// Texture array for base color
layout(binding = 1) uniform sampler2D textures[];
layout(binding = 2) uniform sampler2D normalTextures[];
layout(binding = 5) uniform sampler2D metallicRoughnessTextures[];

// G-Buffer outputs
layout(location = 0) out vec4 outAlbedo;   // Albedo (RGBA, VK_FORMAT_R8G8B8A8_UNORM)
layout(location = 1) out vec4 outNormal;   // Normal.xyz (RGB16F, VK_FORMAT_R16G16B16A16_SFLOAT)
layout(location = 2) out vec2 outParams;   // Roughness/Metallic (RG8, VK_FORMAT_R8G8_UNORM)

// Normal map intensity - you can make this a uniform if needed
const float normalMapStrength = 1.0;

void main() {
    // --- Albedo + alpha test ---
    vec4 albedo = texture(textures[baseColorTexture], vTexCoord);
    if (albedo.a < 0.95) discard;
    outAlbedo = albedo;

    // --- Determine normal ---
    vec3 worldNormal = normalize(vNormal);

    if (normalTextureIndex < textureCount) {
        // 1) Sample normal map with proper mipmapping
        vec3 tspaceNormal = textureLod(normalTextures[normalTextureIndex], vTexCoord, 1.0).xyz;
        tspaceNormal = tspaceNormal * 2.0 - 1.0;

        // 2) Apply adjustable strength with normalization
        tspaceNormal.xy *= normalMapStrength;
        tspaceNormal = normalize(tspaceNormal);

        // 3) Use pre-calculated TBN vectors from vertex shader
        vec3 T = normalize(vTangent.xyz);
        vec3 B = normalize(cross(worldNormal, T) * vTangent.w);
        vec3 N = worldNormal;

        // 4) Build robust TBN matrix
        mat3 TBN = mat3(T, B, N);

        // 5) Apply with fallback to geometric normal
        vec3 mappedNormal = TBN * tspaceNormal;
        float lenSq = dot(mappedNormal, mappedNormal);
        worldNormal = (lenSq > 0.001) ? normalize(mappedNormal) : worldNormal;
    }

    // 6) Encode with reduced precision (avoids storage artifacts)
    outNormal = vec4(worldNormal * 0.5 + 0.5, 1.0);

    // --- Metallic / Roughness ---
    vec4 mr = texture(metallicRoughnessTextures[metalRoughTextureIndex], vTexCoord);
    float roughness = clamp(mr.g, 0.04, 1.0);
    float metallic  = clamp(mr.b, 0.0, 1.0);
    outParams = vec2(roughness, metallic);
}
