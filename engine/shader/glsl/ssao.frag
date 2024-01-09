#version 450
#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"

layout (constant_id = 0) const int SSAO_KERNEL_SIZE = 64;
layout (constant_id = 1) const float SSAO_RADIUS = 0.5;
layout (binding = 0) uniform sampler2D inputViewPosition;
layout (binding = 1) uniform sampler2D inputNormal;
layout (binding = 2) uniform sampler2D ssaoNoise;

layout (binding = 3) uniform UBOSSAOKernel
{
    vec4 samples[SSAO_KERNEL_SIZE];
} uboSSAOKernel;

layout (binding = 4) uniform UBO
{
    mat4 projection;
} ubo;

layout (location = 0) in vec2 inUV;
layout (location = 0) out float outFragColor;
void main()
{
    vec3 pos = texture(inputViewPosition, inUV).xyz;
    vec3 normal = texture(inputNormal, inUV).rgb;

    ivec2 texDim = textureSize(inputViewPosition, 0);
    ivec2 noiseDim = textureSize(ssaoNoise, 0);
    const vec2 noiseUV = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y)) * inUV;
    vec3 randomVec = texture(ssaoNoise, noiseUV).xyz * 2.0 - 1.0;

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(tangent, normal);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.f;
    const float bias = 0.025f;
    int validCnt = 0;
    for (int i=0;i<SSAO_KERNEL_SIZE;++i){
        vec3 samplePos = TBN * uboSSAOKernel.samples[i].xyz;
        samplePos = pos + samplePos * SSAO_RADIUS;
        vec4 offset = ubo.projection * vec4(samplePos, 1.0f);

        offset.xyz= offset.xyz / offset.w * 0.5f + 0.5f;
        if(offset.x<0||offset.y<0)continue;
        ++validCnt;
        float sampleDepth = -texture(inputViewPosition, offset.xy).w;
//        if(offset.y<0)
//        debugPrintfEXT("vec3:%f,%f,%f;%f,%f,%f %f %f\n", offset.x, offset.y, offset.z, samplePos.x, samplePos.y, samplePos.z
//        ,sampleDepth,samplePos.z + bias);

        float rangeCheck = smoothstep(0.f, 1.f, SSAO_RADIUS/abs(pos.z-sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0f : 0.0f) * rangeCheck;
    }
    occlusion = 1.0-(occlusion/float(validCnt));
    outFragColor = occlusion;
}
