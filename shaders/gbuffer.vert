#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_scalar_block_layout : require

// Push‑constants mirror your C++ PushConstants struct
layout(push_constant, scalar) uniform PushConstants {
    uint64_t vertexBufferAddress;  // SSBO address for vertex pulling
    uint32_t baseColorTextureIndex;
    uint32_t metalRoughTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t textureCount;
    vec3     modelScale;           // Scale applied in model space
} pc;

struct Vertex {
    vec3 pos;      // Model‑space position
    vec2 texCoord; // UV
    vec3 normal;   // Model‑space normal
    vec4 tangent;  // Tangent.xyz + bitangent sign in w
};

// Buffer‑reference for vertex pulling
layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex vertices[];
};

// UBO for transforms
layout(binding = 0, scalar) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} ubo;

// Outputs to the fragment stage
layout(location = 0) out vec3  vWorldPos;
layout(location = 1) out vec3  vNormal;
layout(location = 2) out vec2  vTexCoord;
layout(location = 3) out vec4  vTangent;
layout(location = 4) flat out uint baseColorTexture;
layout(location = 5) flat out uint metalRoughTextureIndex;
layout(location = 6) flat out uint normalTextureIndex;
layout(location = 7) flat out uint textureCount;

void main() {
    // --- MATERIALS ---
    baseColorTexture        = pc.baseColorTextureIndex;
    metalRoughTextureIndex  = pc.metalRoughTextureIndex;
    normalTextureIndex      = pc.normalTextureIndex;
    textureCount            = pc.textureCount;

    // --- VERTICES ---
    VertexBuffer vb = VertexBuffer(pc.vertexBufferAddress);
    Vertex v       = vb.vertices[gl_VertexIndex];

    // Scale & world‐transform position
    vec3 scaledPos   = v.pos * pc.modelScale;
    vec4 worldPos4   = ubo.model * vec4(scaledPos, 1.0);
    vWorldPos        = worldPos4.xyz;

    // --- NORMAL MATRIX ---
    mat3 model3      = mat3(ubo.model);
    mat3 normalMatrix = transpose(inverse(model3));

    // Transform normal & tangent
    vNormal  = normalize(mat3(ubo.model) * v.normal);
    vec3 t    = normalize(normalMatrix * v.tangent.xyz);
    vTangent = vec4(t, v.tangent.w);

    // Pass UV
    vTexCoord = v.texCoord;

    // --- FINAL POSITION ---
    gl_Position = ubo.proj * ubo.view * worldPos4;
}
