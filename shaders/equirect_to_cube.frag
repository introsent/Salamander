#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout(push_constant, scalar) uniform PushConstants {
    uint64_t vertexBufferAddress;
    mat4 viewProj;
    uint faceIndex;
} pc;

layout(binding = 0) uniform sampler2D equirectTexture;

layout(location = 0) in vec3 fragLocalPosition;
layout(location = 0) out vec4 outColor;

const vec2 invAtan = vec2(0.1591, 0.3183); // 1/(2*PI), 1/PI

vec2 sampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec3 direction = normalize(fragLocalPosition);

    direction = vec3(direction.z, direction.y, -direction.x);
    vec2 uv = sampleSphericalMap(direction);
    outColor = texture(equirectTexture, uv);
}