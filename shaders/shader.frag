#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 1) uniform sampler2D textures[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint fragMaterialIndex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textures[fragMaterialIndex], fragTexCoord);
}