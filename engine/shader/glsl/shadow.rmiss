#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_debug_printf : enable
struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
	bool shadowd;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
//struct RayPayload {
//	vec3 color;
//	float distance;
//	vec3 normal;
//	float reflector;
//	bool shadowd;
//};
//struct ShadowPayload{
//    vec3 color;
//    int count;
//};
//layout(location = 0) rayPayloadEXT RayPayload rayPayload;
//layout(location = 1) rayPayloadEXT ShadowPayload shadowPayload;

//layout(location = 2) rayPayloadEXT bool shadowd;
void main()
{
    rayPayload.shadowd = false;
}