#version 450
#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"

layout (input_attachment_index = 0,set = 0,binding = 0) uniform subpassInput inputShadow;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec4 shadow = subpassLoad(inputShadow);

    outFragColor = shadow;
}
