#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_scalar_block_layout : require

// Define Vertex structure
struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 texCoord;
};


// Push constant
layout(push_constant) uniform PushConstants {
    uint64_t vertexBufferAddress; // 8 bytes
} pushConstants;


// Declare VertexBuffer with scalar packing
layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    Vertex vertices[];
};
// UBO (std140 for C++ alignment)
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Outputs
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    VertexBuffer vertexBuffer = VertexBuffer(pushConstants.vertexBufferAddress);
    Vertex v = vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(v.pos, 1.0);
    fragColor = v.color;
    fragTexCoord = v.texCoord;
}