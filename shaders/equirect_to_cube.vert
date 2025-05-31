#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout(push_constant, scalar) uniform PushConstants {
    uint64_t vertexBufferAddress;
    mat4 viewProj;
    uint faceIndex;
} pc;

layout(buffer_reference, scalar) readonly buffer Vertices {
    vec3 positions[];
};

layout(location = 0) out vec3 fragLocalPosition;

void main() {
    Vertices vertices = Vertices(pc.vertexBufferAddress);
    vec3 pos = vertices.positions[gl_VertexIndex];
    fragLocalPosition = pos;
    gl_Position = pc.viewProj * vec4(pos, 1.0);
}