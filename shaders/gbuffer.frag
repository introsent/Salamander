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
    vec3 worldNormal = normalize(vNormal); // Start with geometric normal

    if (normalTextureIndex < textureCount) {
        // 1) Sample the tangent-space normal map
        vec3 tspaceNormal = texture(normalTextures[normalTextureIndex], vTexCoord).xyz * 2.0 - 1.0;

        // Optional: adjust normal intensity
        tspaceNormal.xy *= normalMapStrength;
        tspaceNormal = normalize(tspaceNormal);

        // 2) Reconstruct T, B, N in world-space
        vec3 N = worldNormal;
        vec3 T = normalize(vTangent.xyz - dot(vTangent.xyz, N) * N); // Orthogonalize T to N
        vec3 B = cross(N, T) * vTangent.w; // Correct bitangent with handedness

        // 3) Build TBN matrix and transform
        mat3 TBN = mat3(T, B, N);
        worldNormal = normalize(TBN * tspaceNormal);
    }

    // --- Store world-space normal into RGB channels ---
    // Encode to [0..1] range for storage
    outNormal = vec4(worldNormal * 0.5 + 0.5, 1.0);

    // --- Metallic / Roughness ---
    vec4 mr = texture(metallicRoughnessTextures[metalRoughTextureIndex], vTexCoord);
    float roughness = clamp(mr.g, 0.04, 1.0);
    float metallic  = clamp(mr.b, 0.0, 1.0);
    outParams = vec2(roughness, metallic);
}
