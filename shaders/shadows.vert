#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex {
    vec3 pos;      // Model-space position
    vec2 texCoord; // UV
    vec3 normal;   // Model-space normal
    vec4 tangent;  // Tangent.xyz + bitangent sign in w
};

layout(push_constant, scalar) uniform PushConstants {
    uint64_t vertexBufferAddress;   // GPU address of vertex buffer
    vec3 modelScale;                // Scale factor for the model
    uint32_t baseColorTextureIndex;
} pushConstants;

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(binding = 0, scalar) uniform DirectionalLightData {
    vec3 directionalLightDirection;
    vec3 directionalLightColor;
    float directionalLightIntensity;
    mat4 view;
    mat4 projection;
} directionalLight;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) flat out uint vMaterial;

void main() {
    VertexBuffer vertexBuffer = VertexBuffer(pushConstants.vertexBufferAddress);
    Vertex v = vertexBuffer.vertices[gl_VertexIndex];

    // Apply model scaling and transform to light space
    vec3 scaledPos = v.pos;
    vec4 clipPos = directionalLight.projection * directionalLight.view * vec4(scaledPos, 1.0);

    // PERSPECTIVE DIVIDE: Transform from clip space to NDC
    gl_Position = clipPos;
    gl_Position.xyz /= clipPos.w;  // Perspective divide

    // Pass through texture coordinates and material index for alpha testing
    vTexCoord = v.texCoord;
    vMaterial = pushConstants.baseColorTextureIndex;
}