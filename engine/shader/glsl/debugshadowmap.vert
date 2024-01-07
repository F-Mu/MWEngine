#version 450

#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"
layout(push_constant) uniform PushConsts {
	uint cascadeIndex;
} pushConsts;

layout (location = 0) out vec2 outUV;
layout (location = 1) out uint outCascadeIndex;

void main() 
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	outCascadeIndex = pushConsts.cascadeIndex;
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
