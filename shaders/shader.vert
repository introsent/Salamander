#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 texCoord;
};

layout(push_constant) uniform PushConstants {
    uint64_t vertexBufferAddress;
    uint32_t indexOffset;
    uint32_t materialIndex;
    vec3 modelScale;
} pushConstants;

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out flat uint fragMaterialIndex;

void main() {
    VertexBuffer vertexBuffer = VertexBuffer(pushConstants.vertexBufferAddress);
    Vertex v = vertexBuffer.vertices[gl_VertexIndex];

    vec3 scaledPos = v.pos * pushConstants.modelScale;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(scaledPos, 1.0);

    fragTexCoord = v.texCoord;
    fragMaterialIndex = pushConstants.materialIndex;

}
