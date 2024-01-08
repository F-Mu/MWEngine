#version 450

#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;

layout (binding = 0) uniform UBO
{
    mat4 projection;
    mat4 view;
} ubo;

layout(push_constant) uniform PushConsts {
    vec3 position;
} pushConsts;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec3 outTangent;
layout (location = 5) out vec3 outViewPos;

void main()
{
    vec3 tmpPos = inPos + pushConsts.position;

    gl_Position = ubo.projection * ubo.view * vec4(tmpPos,1.0);

    outUV = inUV;

    // Vertex position in world space
    outWorldPos = tmpPos;

    outViewPos = (ubo.view * vec4(tmpPos,1.0)).xyz;

    // Normal in world space
//    mat3 mNormal = transpose(inverse(mat3(ubo.model)));
//    outNormal = mNormal * normalize(inNormal);
//    outTangent = mNormal * normalize(inTangent);

    outNormal = inNormal;
    outTangent = inTangent;
    // Currently just vertex color
}
