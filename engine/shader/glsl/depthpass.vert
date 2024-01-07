#version 450
#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout(push_constant) uniform PushConsts {
    vec3 position;
} pushConsts;

layout (binding = 0) uniform UBO {
    mat4 cascadeProjViewMat;
} ubo;

layout (location = 0) out vec2 outUV;

void main()
{
    outUV = inUV;
    vec3 pos = inPos + pushConsts.position;
    gl_Position = ubo.cascadeProjViewMat * vec4(pos, 1.0);
}