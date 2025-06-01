#version 450
#extension GL_EXT_scalar_block_layout : require

layout(binding = 0, scalar) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} ubo;

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
layout(binding = 6) uniform samplerCube gCubeMap;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

// PBR Functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (3.141592653589793 * denom * denom);
}

float GeometrySchlickGGX_Direct(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySchlickGGX_Indirect(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.8; // Different coefficient for IBL
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness, bool indirect) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = indirect ?
    GeometrySchlickGGX_Indirect(NdotV, roughness) :
    GeometrySchlickGGX_Direct(NdotV, roughness);

    float ggx1 = indirect ?
    GeometrySchlickGGX_Indirect(NdotL, roughness) :
    GeometrySchlickGGX_Direct(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) *
    pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Calculate light contribution with PBR
vec3 calculatePBRLighting(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic,
float roughness, vec3 radiance, bool indirect) {
    vec3 H = normalize(V + L);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness, indirect);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    // Diffuse term with energy conservation
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / 3.141592653589793;

    // Final light contribution
    float NdotL = max(dot(N, L), 0.0);
    return (diffuse + specular) * radiance * NdotL;
}

float calculatePointLightAttenuation(vec3 lightPos, vec3 fragPos, float lightRadius) {
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (4.0 * 3.141592653589793 * distance * distance);

    if (lightRadius > 0.0) {
        attenuation *= max(0.0, 1.0 - pow(distance / lightRadius, 4.0));
    }

    return attenuation;
}

vec3 GetWorldPositionFromDepth(
float depth,
ivec2 fragCoords,
mat4 invProj,
mat4 invView,
ivec2 resolution
) {
    vec2 ndc;
    ndc.x = (float(fragCoords.x) / float(resolution.x)) * 2.0 - 1.0;
    ndc.y = (float(fragCoords.y) / float(resolution.y)) * 2.0 - 1.0;

    vec4 clipPos = vec4(ndc, depth, 1.0);
    vec4 viewPos = invProj * clipPos;
    viewPos /= viewPos.w;
    vec4 worldPos = invView * viewPos;
    return worldPos.xyz;
}

void main() {
    ivec2 resolution = textureSize(gDepth, 0);
    mat4 invView = inverse(ubo.view);
    mat4 invProj = inverse(ubo.proj);
    ivec2 texCoord = ivec2(gl_FragCoord.xy);

    // Sample G-buffer
    vec3 albedo = texelFetch(gAlbedo, texCoord, 0).rgb;
    vec3 encodedNormal = texelFetch(gNormal, texCoord, 0).rgb;
    vec2 params = texelFetch(gParams, texCoord, 0).rg;
    float depth = texelFetch(gDepth, texCoord, 0).r;

    vec3 N = normalize(encodedNormal * 2.0 - 1.0);
    float roughness = params.x;
    float metallic = params.y;
    vec3 worldPos = GetWorldPositionFromDepth(depth, texCoord, invProj, invView, resolution);
    vec3 V = normalize(ubo.cameraPosition - worldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Direct lighting
    vec3 Lo = vec3(0.0);

    // Directional light
    vec3 L_dir = normalize(-vec3(0.577, -0.577, -0.577));
    vec3 radiance_dir = vec3(1.0) * 10000.0;
    Lo += calculatePBRLighting(N, V, L_dir, albedo, metallic, roughness, radiance_dir, false);

    // Point light
    vec3 L_point = normalize(lightData.pointLightPosition - worldPos);
    float attenuation = calculatePointLightAttenuation(
    lightData.pointLightPosition,
    worldPos,
    lightData.pointLightRadius
    );
    vec3 radiance_point = lightData.pointLightColor * lightData.pointLightIntensity * attenuation;
    Lo += calculatePBRLighting(N, V, L_point, albedo, metallic, roughness, radiance_point, false);

    // Indirect lighting (diffuse irradiance)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    // Sample irradiance cubemap (flip Y for coordinate system conversion)
    vec3 irradiance = texture(gCubeMap, vec3(N.x, -N.y, N.z)).rgb;

    // Calculate ambient diffuse
    vec3 diffuse = irradiance * albedo;
    vec3 ambient = kD * diffuse;

    // Final color (direct + indirect)
    vec3 color = ambient + Lo;
    outColor = vec4(irradiance, 1.0);
}