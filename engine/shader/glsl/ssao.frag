#version 450
#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"
layout (input_attachment_index = 0,set = 1,binding = 0) uniform subpassInput inputPosition;
layout (input_attachment_index = 1,set = 1,binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2,set = 1,binding = 2) uniform subpassInput inputAlbedo;
layout (input_attachment_index = 3,set = 1,binding = 3) uniform subpassInput inputViewPos;

void main()
{
    vec3 inPos = subpassLoad(inputPosition).xyz;
    vec3 inNormal = subpassLoad(inputNormal).rgb;
    vec4 color = subpassLoad(inputAlbedo);
    vec3 inViewPos = subpassLoad(inputViewPos).xyz;

}
