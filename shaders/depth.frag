#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// Inputs from vertex shader
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) flat in uint vMaterial;

// Texture array for base color
layout(binding = 1) uniform sampler2D textures[];

void main() {
    // Sample the texture
    vec4 albedo = texture(textures[vMaterial], vTexCoord);

    // Alpha cutout
    if (albedo.a < 0.95) {  // Adjust the threshold as needed
        discard;
    }

    // No need to write to any outputs, as this is a depth pre-pass
}
