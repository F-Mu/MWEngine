#version 450

#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"
layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO
{
    mat4 projection;
} ubo;

layout(location = 0) out vec3 outUVW;

void main()
{
    outUVW = inPos;
    gl_Position = ubo.projection * vec4(inPos.xyz, 1.0);
}
