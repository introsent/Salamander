#version 450
#extension GL_EXT_scalar_block_layout : require

layout(binding = 0, scalar) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
} ubo;

layout(binding = 5, scalar) uniform PointLightData {
    vec3 pointLightPosition;
    float pointLightIntensity;  // In lumens
    vec3 pointLightColor;
    float pointLightRadius;
} pointLight;

layout(binding = 8, scalar) uniform DirectionalLightData {
    vec3 directionalLightPosition;
    vec3 directionalLightDirection;
    vec3 directionalLightColor;
    float directionalLightIntensity;  // In lux (lumens/m²)
    mat4 view;
    mat4 projection;
} directionalLight;

// G-buffer textures
layout(binding = 1) uniform sampler2D gAlbedo;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform sampler2D gParams;
layout(binding = 4) uniform sampler2D gDepth;
layout(binding = 6) uniform samplerCube gCubeMap;
layout(binding = 7) uniform samplerCube gIrradianceMap;
layout(binding = 9) uniform sampler2DShadow gShadowMap;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

const float PI = 3.141592653589793;
const float MAX_REFLECTION_LOD = 4.0; // Adjust based on your prefiltered mip levels

// PBR Functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float GeometrySchlickGGX_Direct(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySchlickGGX_IBL(float NdotV, float roughness) {
    float k = (roughness * roughness) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness, bool isIBL) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2, ggx1;
    if (isIBL) {
        ggx2 = GeometrySchlickGGX_IBL(NdotV, roughness);
        ggx1 = GeometrySchlickGGX_IBL(NdotL, roughness);
    } else {
        ggx2 = GeometrySchlickGGX_Direct(NdotV, roughness);
        ggx1 = GeometrySchlickGGX_Direct(NdotL, roughness);
    }

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) *
    pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Calculate direct light contribution with PBR
vec3 calculateDirectLighting(vec3 N, vec3 V, vec3 L, vec3 albedo,
float metallic, float roughness, vec3 radiance) {
    vec3 H = normalize(V + L);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness, false); // false = direct lighting
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // Energy conservation: kD is reduced by both Fresnel and metallicness
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    float NdotL = max(dot(N, L), 0.0);
    return (diffuse + specular) * radiance * NdotL;
}

float calculatePointLightAttenuation(vec3 lightPos, vec3 fragPos, float lightRadius) {
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / max(distance * distance, 0.0001);

    // Optional: soft cutoff at radius to avoid artifacts
    float cutoff = 1.0 - smoothstep(lightRadius * 0.8, lightRadius, distance);
    return attenuation * cutoff;
}

float ShadowCalculation(vec3 worldPos) {
    vec4 lightSpacePos = directionalLight.projection * directionalLight.view * vec4(worldPos, 1.0);
    lightSpacePos.xyz /= lightSpacePos.w;

    vec3 shadowUV = vec3(lightSpacePos.xy * 0.5 + 0.5, lightSpacePos.z);
    shadowUV.y = 1.0 - shadowUV.y;

    // Check if position is within shadow map bounds
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 ||
    shadowUV.y < 0.0 || shadowUV.y > 1.0 ||
    shadowUV.z > 1.0) {
        return 1.0; // Outside shadow map = fully lit
    }

    return texture(gShadowMap, shadowUV);
}

vec3 GetWorldPositionFromDepth(float depth, ivec2 fragCoords, mat4 invProj,
mat4 invView, ivec2 resolution) {
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
    const float envIntensity = 5000.0; // Adjustable ( based on HDR environment )

    ivec2 resolution = textureSize(gDepth, 0);
    mat4 invView = inverse(ubo.view);
    mat4 invProj = inverse(ubo.proj);
    ivec2 texCoord = ivec2(gl_FragCoord.xy);

    // Sample G-buffer
    vec3 albedo = texelFetch(gAlbedo, texCoord, 0).rgb;
    vec3 encodedNormal = texelFetch(gNormal, texCoord, 0).rgb;
    vec2 params = texelFetch(gParams, texCoord, 0).rg;
    float depth = texelFetch(gDepth, texCoord, 0).r;

    // Render skybox for background pixels
    if (depth >= 1.0) {
        vec3 worldPos = GetWorldPositionFromDepth(1.0, texCoord, invProj, invView, resolution);
        vec3 sampleDirection = normalize(worldPos);
        outColor = vec4(texture(gCubeMap, sampleDirection).rgb * envIntensity, 1.0);
        return;
    }

    // Decode G-buffer data
    vec3 N = normalize(encodedNormal * 2.0 - 1.0);
    float roughness = params.x;
    float metallic = params.y;
    vec3 worldPos = GetWorldPositionFromDepth(depth, texCoord, invProj, invView, resolution);
    vec3 V = normalize(ubo.cameraPosition - worldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // DIRECT LIGHTING

    vec3 Lo = vec3(0.0);

    // Directional light
    vec3 L_dir = normalize(-directionalLight.directionalLightDirection);
    float shadowTerm = ShadowCalculation(worldPos);
    vec3 radiance_dir = directionalLight.directionalLightColor *
    directionalLight.directionalLightIntensity;
    Lo += shadowTerm * calculateDirectLighting(N, V, L_dir, albedo,
    metallic, roughness, radiance_dir);

    // Point light
    vec3 L_point = normalize(pointLight.pointLightPosition - worldPos);
    float attenuation = calculatePointLightAttenuation(
    pointLight.pointLightPosition,
    worldPos,
    pointLight.pointLightRadius
    );
    // Convert lumens to radiance
    vec3 radiance_point = pointLight.pointLightColor * attenuation *
    (pointLight.pointLightIntensity / (4.0 * PI));
    Lo += calculateDirectLighting(N, V, L_point, albedo,
    metallic, roughness, radiance_point);


    // INDIRECT LIGHTING (IBL)

    // Calculate Fresnel for indirect lighting
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    // Energy conservation
    vec3 kS = F;  // Specular contribution
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);  // Diffuse contribution

    // Diffuse IBL
    vec3 irradiance = texture(gIrradianceMap, vec3(N.x, -N.y, N.z)).rgb;
    vec3 diffuseIBL = kD * irradiance * albedo;

    vec3 ambient = diffuseIBL * envIntensity;


    // FINAL COLOR
    vec3 color = Lo + ambient;

    outColor = vec4(color, 1.0);
}