#version 450

layout(push_constant) uniform PushConstants {
    uint64_t vertexBufferAddress;
    mat4 viewProj;
    uint faceIndex;
} pc;

layout(binding = 0) uniform sampler2D equirectTexture;

layout(location = 0) out vec4 outColor;

const vec2 invAtan = vec2(0.1591, 0.3183); // 1/(2*PI), 1/PI

vec2 sampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    // Reconstruct world position from clip space
    vec4 worldPos = inverse(pc.viewProj) * vec4(gl_FragCoord.xy, 0.0, 1.0);
    worldPos.xyz /= worldPos.w;
    vec3 dir = normalize(worldPos.xyz);

    // Convert direction to spherical coordinates
    vec2 uv = sampleSphericalMap(dir);
    outColor = texture(equirectTexture, uv);
}