#version 450

#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"
layout (set = 1, binding = 0) uniform sampler2D colorMap;

layout (location = 0) in vec2 inUV;

void main() 
{
	float alpha = texture(colorMap, inUV).a;
	if (alpha < 0.5) {
		discard;
	}
}