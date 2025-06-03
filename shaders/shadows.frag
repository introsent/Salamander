#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) flat in uint vMaterial;

layout(binding = 1) uniform sampler2D textures[];

void main() {
    // Alpha cutout (same as depth prepass)
    vec4 albedo = texture(textures[vMaterial], vTexCoord);
    if (albedo.a < 0.95) {
        discard;
    }
}
