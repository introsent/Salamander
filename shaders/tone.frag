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
layout(set = 0, binding = 2) uniform sampler2D avgLuminanceTex;

layout(location = 0) out vec4 outColor;

// convert average luminance to EV100
float luminanceToEV100(float avgLum) {
    // EV100 = log2(avgLum * 100 / 12.5)
    // 12.5 is the reflected light meter calibration constant
    const float K = 12.5;
    return log2(avgLum * 100.f / K);
}

// compute exposure from EV100
float ev100ToExposure(float ev100) {
    // exposure = 1 / (1.2 * 2^EV100)
    return 1.0 / (1.2 * pow(2.0, ev100));
}

float computeManualExposure(float aperture, float shutterSpeed, float ISO) {
    float ev100 = log2((aperture * aperture) / shutterSpeed * 100.0 / ISO);
    return ev100ToExposure(ev100);
}

void main() {
    // sample HDR color
    vec2 uv = gl_FragCoord.xy / pc.screenSize;
    vec3 hdr = texture(hdrTex, uv).rgb;

    // get average luminance from 1x1 texture
    float avgLum = texture(avgLuminanceTex, vec2(0.5, 0.5)).r;

    // compute exposure
    float exposure;
    //if (camExp.ev100Override >= 0.0) {
    //    // manual override
    //    exposure = ev100ToExposure(camExp.ev100Override);
    //} else if (camExp.aperture > 0.0) {
    //    // manual camera settings
    //    exposure = computeManualExposure(camExp.aperture, camExp.shutterSpeed, camExp.ISO);
    //} else {
    //    // automatic exposure from average luminance
    //    float ev100 = luminanceToEV100(avgLum);
    //    exposure = ev100ToExposure(ev100);
    //}
    float ev100 = luminanceToEV100(avgLum);
    exposure = ev100ToExposure(ev100);

    // apply exposure
    vec3 mapped = hdr * exposure;

    // reinhard tone mapping
    vec3 ldr = mapped / (mapped + vec3(1.0));

    outColor = vec4(ldr, 1.0);
}