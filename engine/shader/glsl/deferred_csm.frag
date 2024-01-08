#version 450
#extension GL_GOOGLE_include_directive : enable
#include "debug.glsl"

#define SHADOW_MAP_CASCADE_COUNT 8

layout (input_attachment_index = 0,set = 2,binding = 0) uniform subpassInput inputPosition;
layout (input_attachment_index = 1,set = 2,binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2,set = 2,binding = 2) uniform subpassInput inputAlbedo;
layout (input_attachment_index = 3,set = 2,binding = 3) uniform subpassInput inputViewPos;
layout (set = 0, binding = 1) uniform sampler2DArray shadowMap;
layout (set = 1, binding = 0) uniform sampler2D colorMap;

layout (location = 0) in vec3 inUV;

//layout (constant_id = 0) const int enablePCF = 0;

layout (location = 0) out vec4 outFragColor;

#define ambient 0.3

layout (set = 0, binding = 2) uniform UBO {
    vec4 cascadeSplits[SHADOW_MAP_CASCADE_COUNT/4];
    mat4 cascadeViewProjMat[SHADOW_MAP_CASCADE_COUNT];
    mat4 inverseViewMat;
    vec3 lightDir;
    float _pad;
    int colorCascades;
} ubo;

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0
);

float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
    float shadow = 1.0;
    float bias = 0.005;

    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
        float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
        if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
            shadow = ambient;
        }
    }
    return shadow;

}

float filterPCF(vec4 sc, uint cascadeIndex)
{
    ivec2 texDim = textureSize(shadowMap, 0).xy;
    vec2 offset = 1.0 / texDim;
    //	float dx = 1.0 / float(texDim.x);
    //	float dy = 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++) {
        for (int y = -range; y <= range; y++) {
            shadowFactor += textureProj(sc, vec2(x, y) * offset, cascadeIndex);
            count++;
        }
    }
    return shadowFactor / count;
}

void main()
{
    vec3 inPos = subpassLoad(inputPosition).xyz;
    vec3 inNormal = subpassLoad(inputNormal).rgb;
    vec4 color = subpassLoad(inputAlbedo);
    vec3 inViewPos = subpassLoad(inputViewPos).xyz;

    if (color.a < 0.5) {
        discard;
    }

    // Get cascade index for the current fragment's view position
    uint cascadeIndex = 0;
    for (int i = SHADOW_MAP_CASCADE_COUNT - 1; i >= 0; --i) {
        if (inViewPos.z < ubo.cascadeSplits[i/4][i%4]) {
            cascadeIndex = i + 1;
            break;
        }
    }

    // Depth compare for shadowing
    vec4 shadowCoord = (biasMat * ubo.cascadeViewProjMat[cascadeIndex]) * vec4(inPos, 1.0);

    float shadow = 0;
    //	if (enablePCF == 1) {
    shadow = filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
    //	} else {
    //		shadow = textureProj(shadowCoord / shadowCoord.w, vec2(0.0), cascadeIndex);
    //	}

    // Directional light
    vec3 N = normalize(inNormal);
    vec3 L = normalize(-ubo.lightDir);
    vec3 H = normalize(L + inViewPos);
    float diffuse = max(dot(N, L), ambient);
    vec3 lightColor = vec3(1.0);
    outFragColor.rgb = max(lightColor * (diffuse * color.rgb), vec3(0.0));
    outFragColor.rgb *= shadow;
    outFragColor.a = color.a;

    // Color cascades (if enabled)
    if (ubo.colorCascades == 1) {
        switch (cascadeIndex) {
            case 0 :
            outFragColor.rgb *= vec3(0, 0, 0.25f);
            break;
            case 1 :
            outFragColor.rgb *= vec3(0, 0.25f, 0);
            break;
            case 2 :
            outFragColor.rgb *= vec3(0, 0.25f, 0.25f);
            break;
            case 3 :
            outFragColor.rgb *= vec3(1.0f, 0, 0);
            break;
            case 4 :
            outFragColor.rgb *= vec3(1.0f, 0, 0.25f);
            break;
            case 5 :
            outFragColor.rgb *= vec3(1.0f, 0.25f, 0);
            break;
            case 6 :
            outFragColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
            break;
            case 7 :
            outFragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
            break;
        }
    }
}
