#version 450
#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputShadow;
layout (input_attachment_index = 1, set = 1, binding = 0) uniform subpassInput inputAO;
layout (input_attachment_index = 2, set = 2, binding = 0) uniform subpassInput inputLighting;
layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;
layout (set = 3, binding = 0)uniform UBOParams {
    float exposure;
    float gamma;
} uboParams;

//layout (set = 2, binding = 2) uniform samplerCube samplerEnv;
vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
void main()
{
    vec3 shadow = subpassLoad(inputShadow).rgb;
    float ao = subpassLoad(inputAO).r;
    vec3 lighting = subpassLoad(inputLighting).rgb;
    vec3 color = lighting;

    // Tone mapping
    color = Uncharted2Tonemap(color * uboParams.exposure);
    color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
    // Gamma correction
    color = pow(color, vec3(1.0f / uboParams.gamma));

    outFragColor = vec4(color, 1.0);
}
