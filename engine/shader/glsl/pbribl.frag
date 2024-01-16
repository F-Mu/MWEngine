#version 450
#extension GL_GOOGLE_include_directive : enable
#include "constants.glsl"
#include "mesh_lighting.glsl"
layout (binding = 0) uniform UBOParams {
	vec4 lights[maxLightsCount];
	mat4 projViewMatrix;
	highp vec3 cameraPos;
} uboParams;
layout (binding = 2) uniform samplerCube samplerIrradiance;
layout (binding = 3) uniform sampler2D samplerBRDFLUT;
layout (binding = 4) uniform samplerCube prefilteredMap;
layout (input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputMaterial;
layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inputAlbedo;
layout (input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput inputViewPos;
layout (input_attachment_index = 4, set = 1, binding = 4) uniform subpassInput inputDepth;

layout (location = 0) out vec4 outColor;

#define PI 3.1415926535897932384626433832795
#define ALBEDO vec3(albedo.r, albedo.g, albedo.b)

vec3 prefilteredReflection(vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
	vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

void main()
{
	highp vec3 inPos;
	highp float depth = subpassLoad(inputDepth).r;
	{
		highp vec4  ndc                      = vec4(uv_to_ndcxy(inUV), depth, 1.0);
		highp mat4  inverseProjViewMatrix    = inverse(ubo.projViewMatrix);
		highp vec4  in_world_position_with_w = inverseProjViewMatrix * ndc;
		inPos                                = in_world_position_with_w.xyz / in_world_position_with_w.www;
	}
	//    vec3 inPos = subpassLoad(inputPosition).xyz;
	vec3 metarial = subpassLoad(inputMaterial).rgb;
	vec3 inNormal = subpassLoad(inputNormal).rgb;
	vec4 albedo = subpassLoad(inputAlbedo);
	vec3 inViewPos = subpassLoad(inputViewPos).xyz;
	vec3 N = normalize(inNormal);
	vec3 V = normalize(ubo.camPos - inPos);
	vec3 R = reflect(-V, N); 

	float metallic = material.r;
	float roughness = material.g;

	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, ALBEDO, metallic);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < uboParams.lights[i].length(); i++) {
		vec3 L = normalize(uboParams.lights[i].xyz - inPos);
		Lo += BRDF(L, V, N, F0, ALBEDO, metallic, roughness);
	}   
	
	vec2 brdf = texture(samplerBRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 reflection = prefilteredReflection(R, roughness).rgb;	
	vec3 irradiance = texture(samplerIrradiance, N).rgb;

	// Diffuse based on irradiance
	vec3 diffuse = irradiance * ALBEDO;	

	vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);

	// Specular reflectance
	vec3 specular = reflection * (F * brdf.x + brdf.y);

	// Ambient part
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	  
	vec3 ambient = (kD * diffuse + specular);
	
	vec3 color = ambient + Lo;

	outColor = vec4(color, 1.0);
}