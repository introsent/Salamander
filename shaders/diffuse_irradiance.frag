#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout(push_constant, scalar) uniform PushConstants {
    uint64_t vertexBufferAddress;
    mat4 viewProj;
    uint faceIndex;
} pc;

layout(binding = 0) uniform samplerCube environmentMap;

layout(location = 0) in vec3 fragLocalPosition;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
const float SAMPLE_DELTA = 0.025;

// Tangent space calculation
void TangentToWorld(vec3 N, out vec3 T, out vec3 B) {
    vec3 up = abs(N.y) < 0.9999999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(cross(N, up));
    B = normalize(cross(T, N));
}

void main() {
    vec3 normal = normalize(fragLocalPosition);

    vec3 tangent, bitangent;
    TangentToWorld(normal, tangent, bitangent);

    vec3 irradiance = vec3(0.0);
    float nrSamples = 0.0;

    for(float phi = 0.0; phi < 2.0 * PI; phi += SAMPLE_DELTA) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += SAMPLE_DELTA) {
            // Spherical to cartesian (tangent space)
            vec3 tangentSample = vec3(
            sin(theta) * cos(phi),
            sin(theta) * sin(phi),
            cos(theta)
            );

            // Tangent to world space
            vec3 sampleVec =
            tangentSample.x * tangent +
            tangentSample.y * bitangent +
            tangentSample.z * normal;

            irradiance += texture(environmentMap, sampleVec).rgb *
            cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    outColor = vec4(irradiance, 1.0);
}