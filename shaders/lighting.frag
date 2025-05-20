#version 450

layout(binding = 1) uniform sampler2D gAlbedo;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform sampler2D gParams;
layout(binding = 4) uniform sampler2D gDepth;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    // Sample G-buffer textures
    vec3 albedo = texture(gAlbedo, fragUV).rgb;
    vec3 encodedNormal = texture(gNormal, fragUV).rgb;  // Now sampling 3 channels
    vec2 params = texture(gParams, fragUV).rg;
    float roughness = params.x;
    float metallic = params.y;

    // Decode normal from [0,1] to [-1,1] range
    vec3 normal = normalize(encodedNormal * 2.0 - 1.0);

    // Simple directional lighting
    vec3 lightDir = normalize(vec3(-0.5, 1.0, 0.3));
    float diff = max(dot(normal, lightDir), 0.0);

    // Basic lighting calculation
    vec3 color = albedo * diff * (1.0 - roughness * 0.5);

    // Add ambient term to avoid completely black shadows
    float ambient = 0.2;
    color += albedo * ambient;

    ivec2 texelCoord = ivec2(fragUV * vec2(textureSize(gDepth, 0)));
    float depthTexelFetch = texelFetch(gDepth, texelCoord, 0).r;
    float depth = depthTexelFetch;
    vec3 debugColor;

    float a = 0.945;  // Lower bound
    float b = 1.0;   // Upper bound
    float remapped = clamp((depth - a) / (b - a), 0.0, 1.0);
    debugColor = vec3(remapped);

    outColor = vec4(debugColor, 1.0);
}