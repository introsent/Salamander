#version 450

layout(set = 0, binding = 0) uniform sampler2D hdrTex;

// Push-constant block for screen dimensions
layout(push_constant) uniform PushConstants {
    vec2 screenSize;
} pc;

// New camera/exposure uniforms (set=0, binding=1)
layout(set = 0, binding = 1) uniform CameraExposure {
    float aperture;      // f-stop, e.g. 2.8
    float shutterSpeed;  // exposure time in seconds, e.g. 1/60.0
    float ISO;           // sensitivity, e.g. 100.0
    float ev100Override; // if >=0, use this EV100 instead of computing
} camExp;

layout(location = 0) out vec4 outColor;

float computeExposure(in float aperture,
                      in float shutterSpeed,
                      in float ISO,
                      in float ev100Override) {
    float ev100 = (ev100Override >= 0.0)
    ? ev100Override
    : log2( pow(aperture, 2) / shutterSpeed * 100 / ISO);
    return 1.0 / max((1.2 * pow(2.f, ev100)), 0.0001f);
}

void main() {
    // 1) Sample HDR color
    vec2 uv = gl_FragCoord.xy / pc.screenSize;
    vec3 hdr = texture(hdrTex, uv).rgb;

    // 2) Compute exposure multiplier
    float exposure = computeExposure(
    camExp.aperture,
    camExp.shutterSpeed,
    camExp.ISO,
    camExp.ev100Override
    );

    // 3) Apply exposure
    vec3 mapped = hdr * exposure;
//
    // 4) Reinhard tone mapping: LDR = mapped / (mapped + 1)
    vec3 ldr = mapped / (mapped + vec3(1.0));


    // 5) Output; let SRGB swapchain handle gamma
    outColor = vec4(ldr, 1.0);
}