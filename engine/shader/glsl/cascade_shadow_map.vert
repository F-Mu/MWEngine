#version 450

#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewPos;
layout (location = 3) out vec3 outPos;
layout (location = 4) out vec2 outUV;

layout(push_constant) uniform PushConsts {
	vec3 position;
} pushConsts;

void main() 
{
	outColor = inColor;
	outNormal = inNormal;
	outUV = inUV;
	vec3 pos = inPos + pushConsts.position;
	outPos = pos;
	outViewPos = (ubo.view * vec4(pos.xyz, 1.0)).xyz;
	gl_Position = ubo.projection * ubo.view * vec4(pos.xyz, 1.0);
}

