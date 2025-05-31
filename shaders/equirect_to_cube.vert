#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(push_constant) uniform PushConstants {
    uint64_t vertexBufferAddress;
    mat4 viewProj;
    uint faceIndex;
} pc;

layout(buffer_reference, scalar) readonly buffer Vertices {
    vec3 positions[];
};

void main() {
    Vertices vertices = Vertices(pc.vertexBufferAddress);
    vec3 pos = vertices.positions[gl_VertexIndex];
    gl_Position = pc.viewProj * vec4(pos, 1.0);
}