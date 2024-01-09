#version 450
#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputShadow;
layout (input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputAO;
layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec4 shadow = subpassLoad(inputShadow);
    float ao = subpassLoad(inputAO).r;

    outFragColor = shadow * ao;
}
