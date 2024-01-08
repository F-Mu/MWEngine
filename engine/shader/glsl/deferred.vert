#version 450

#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"

void main()
{
    vec2 outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
