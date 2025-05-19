#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_scalar_block_layout : require

// Push-constants mirror your C++ PushConstants struct
layout(push_constant, scalar) uniform PushConstants {
    uint64_t vertexBufferAddress;  // SSBO address for vertex pulling
    uint32_t baseColorTextureIndex;
    uint32_t metalRoughTextureIndex;
    vec3     modelScale;           // Scale applied in model space
} pc;


struct Vertex {
    vec3 pos;      // Model-space position
    vec2 texCoord; // UV
    vec3 normal;   // Model-space normal
    vec4 tangent;  // Tangent.xyz + bitangent sign in w
};

// Buffer-reference for vertex pulling
layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex vertices[];
};

// UBO for transforms
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Outputs to the fragment stage
layout(location = 0) out vec3 vWorldPos;     // For depth if needed or lighting
layout(location = 1) out vec3 vNormal;       // Full 3D normal
layout(location = 2) out vec2 vTexCoord;     // UV
layout(location = 3) flat out uint vBaseColorTexture; // Material/texture index
layout(location = 4) out vec4 vTangent;      // Tangent + bitangent sign
layout(location = 5) flat out uint vMetalRoughTextureIndex;

void main() {
    // Pull vertex from SSBO
    VertexBuffer vb = VertexBuffer(pc.vertexBufferAddress);
    Vertex v = vb.vertices[gl_VertexIndex];

    // Scale and transform position into world space
    vec3 scaledPos = v.pos * pc.modelScale;
    vec4 worldPos4 = ubo.model * vec4(scaledPos, 1.0);
    vWorldPos = worldPos4.xyz;

    // Transform normal (drop w component)
    vNormal = normalize((ubo.model * vec4(v.normal, 0.0)).xyz);

    // Pass UV directly
    vTexCoord = v.texCoord;

    // Pass material index to select the right texture array slot
    vBaseColorTexture = pc.baseColorTextureIndex;
    vMetalRoughTextureIndex = pc.metalRoughTextureIndex;

    // Pass tangent + bitangent sign through
    vTangent = v.tangent;

    // Compute clip-space position
    gl_Position = ubo.proj * ubo.view * worldPos4;
}