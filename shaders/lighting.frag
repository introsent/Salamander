#version 450
#extension GL_EXT_scalar_block_layout : require

// Reuse existing UniformBufferObject
layout(binding = 0, scalar) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} ubo;

// Add point light properties to UBO or as a separate uniform buffer
layout(binding = 5, scalar) uniform LightData {
    vec3 pointLightPosition;
    float pointLightIntensity;  // In lumens
    vec3 pointLightColor;
    float pointLightRadius;     // Maximum radius of influence for optimization
} lightData;

// G-buffer textures
layout(binding = 1) uniform sampler2D gAlbedo;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform sampler2D gParams;
layout(binding = 4) uniform sampler2D gDepth;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

// PBR Functions (from LearnOpenGL)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (3.141592653589793 * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Calculate light contribution with PBR
vec3 calculatePBRLighting(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness, vec3 radiance) {
    vec3 H = normalize(V + L);

    // Calculate base reflectivity (F0)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // Avoid division by zero
    vec3 specular = numerator / denominator;

    // Diffuse term with energy conservation
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / 3.141592653589793;

    // Final light contribution
    float NdotL = max(dot(N, L), 0.0);
    return (diffuse + specular) * radiance * NdotL;
}

// Physically based attenuation for point light
// Inverse square law with smooth falloff
float calculatePointLightAttenuation(vec3 lightPos, vec3 fragPos, float lightRadius) {
    float distance = length(lightPos - fragPos);

    // Using inverse square law for physical correctness
    // For lumen to lux conversion: lux = lumen / (4 * PI * distance²)
    float attenuation = 1.0 / (4.0 * 3.141592653589793 * distance * distance);

    // Optional smooth falloff at radius boundary
    if (lightRadius > 0.0) {
        attenuation *= max(0.0, 1.0 - pow(distance / lightRadius, 4.0));
    }

    return attenuation;
}

// Reconstruct world position from depth
vec3 GetWorldPositionFromDepth(
float depth,
ivec2 fragCoords,
mat4 invProj,
mat4 invView,
ivec2 resolution
) {
    // 1) build NDC XY in [–1,1]
    vec2 ndc;
    ndc.x = (float(fragCoords.x) / float(resolution.x)) * 2.0 - 1.0;
    ndc.y = (float(fragCoords.y) / float(resolution.y)) * 2.0 - 1.0;

    // 2) Vulkan uses [0,1] for clip-Z, so use depth directly
    float clipZ = depth;

    // 3) reconstruct clip-space pos
    vec4 clipPos = vec4(ndc, clipZ, 1.0);

    // 4) bring back to view-space
    vec4 viewPos = invProj * clipPos;
    viewPos /= viewPos.w;

    // 5) bring back to world-space
    vec4 worldPos = invView * viewPos;
    return worldPos.xyz;
}

void main() {
    // Get resolution from depth texture size
    ivec2 resolution = textureSize(gDepth, 0);

    // Compute inverse matrices
    mat4 invView = inverse(ubo.view);
    mat4 invProj = inverse(ubo.proj);

    // Compute texel coordinates
    ivec2 texCoord = ivec2(gl_FragCoord.xy);

    // Sample G-buffer textures
    vec3 albedo = texelFetch(gAlbedo, texCoord, 0).rgb;
    vec3 encodedNormal = texelFetch(gNormal, texCoord, 0).rgb; // Assuming 3-channel normal
    vec2 params = texelFetch(gParams, texCoord, 0).rg; // roughness, metallic
    float depth = texelFetch(gDepth, texCoord, 0).r;

    // Decode normal from [0,1] to [-1,1]
    vec3 N = normalize(encodedNormal * 2.0 - 1.0);

    // Extract PBR parameters
    float roughness = params.x;
    float metallic = params.y;

    // Reconstruct world position
    vec3 worldPos = GetWorldPositionFromDepth(depth, texCoord, invProj, invView, resolution);

    // Compute view direction
    vec3 V = normalize(ubo.cameraPosition - worldPos);

    // Initialize final light accumulation
    vec3 Lo = vec3(0.0);

    // 1. Directional Light Contribution
    vec3 L_dir = normalize(-vec3(0.577, -0.577, -0.577)); // Direction to light
    vec3 radiance_dir = vec3(1.0) * 1.0; // Intensity = 1.0 in lux

    Lo += calculatePBRLighting(N, V, L_dir, albedo, metallic, roughness, radiance_dir);

    // 2. Point Light Contribution
    vec3 L_point = normalize(lightData.pointLightPosition - worldPos);
    float attenuation = calculatePointLightAttenuation(lightData.pointLightPosition, worldPos, lightData.pointLightRadius);

    // Convert lumen to lux using attenuation
    vec3 radiance_point = lightData.pointLightColor * lightData.pointLightIntensity * attenuation;

    Lo += calculatePBRLighting(N, V, L_point, albedo, metallic, roughness, radiance_point);

    // Simple Reinhard tonemapping
    vec3 color = Lo / (Lo + vec3(1.0));

    // Output final color
    outColor = vec4(color, 1.0);
}