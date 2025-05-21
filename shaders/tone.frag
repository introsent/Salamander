#version 450

// HDR input bound at set=0, binding=0
layout(set = 0, binding = 0) uniform sampler2D hdrTex;

// Push-constant block for screen dimensions
layout(push_constant) uniform PushConstants {
    vec2 screenSize;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    // Compute UV in [0,1]
    vec2 uv = gl_FragCoord.xy / pc.screenSize;
    vec3 hdr = texture(hdrTex, uv).rgb;

    // Simple Reinhard tone mapping: LDR = HDR / (HDR + 1)
    vec3 ldr = hdr / (hdr + vec3(1.0));

    // No manual gamma; swapchain SRGB format handles that
    outColor = vec4(ldr, 1.0);
}