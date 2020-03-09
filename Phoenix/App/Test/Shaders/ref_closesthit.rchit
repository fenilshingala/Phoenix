#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

layout(location = 0) rayPayloadInNV RayPayload rayPayload;

hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 2, set = 0) uniform UniformBufferObject 
{
	mat4 model;
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
	vec4 camPos;
} ubo;
layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 5, set = 0) uniform sampler2D[] texSampler;

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec3 color;
  vec2 uv;
  float diffuseIndex;
  float shininessIndex;
  float heightIndex;
  float roughnessIndex;
  float pad2;
};

Vertex unpack(uint index)
{
	vec4 d0 = vertices.v[4 * index + 0];		// 4 is no of vec4s stride
	vec4 d1 = vertices.v[4 * index + 1];
	vec4 d2 = vertices.v[4 * index + 2];
	vec4 d3 = vertices.v[4 * index + 3];

	Vertex v;
	v.pos = d0.xyz;
	v.normal = vec3(d0.w, d1.x, d1.y);
	v.color = vec3(d1.z, d1.w, d2.x);
	v.uv = vec2(d2.y, d2.z);
	v.diffuseIndex = d2.w;
	v.shininessIndex = d3.x;
	v.heightIndex = d3.y;
	v.roughnessIndex = d3.z;	// roughness
	return v;
}


const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------

void main()
{
	ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

	Vertex v0 = unpack(index.x);
	Vertex v1 = unpack(index.y);
	Vertex v2 = unpack(index.z);

	// Interpolate normal & uv
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);
	vec2 uv = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;

	//vec3 normal;
	//if(int(v0.diffuseIndex) >= 0)
	//{
	//	normal = texture(texSampler[int(v0.heightIndex)], uv).xyz;
	//}
	//else
	//{
	//	normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);
	//}
	
	// Basic lighting
	vec3 lightVector = normalize(ubo.lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.6);

	vec3 diffuse = vec3(1,1,1);
	if(int(v0.diffuseIndex) >= 0)
	{
		diffuse = texture(texSampler[int(v0.diffuseIndex)], uv).xyz;
	}
	
	vec3 metallic = vec3(0,0,0);
	if(int(v0.shininessIndex) >= 0)
	{
		metallic = texture(texSampler[int(v0.shininessIndex)], uv).xyz;
	}
	
	
	float roughness = 0.0f;
	if(int(v0.roughnessIndex) >= 0)
	{
		roughness = texture(texSampler[int(v0.roughnessIndex)], uv).x;
	}


	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, diffuse, metallic);

	// reflectance equation
    vec3 Lo = vec3(0.0);

	vec3 WorldPos = vec3(ubo.model * vec4(v0.pos, 1.0));
	vec3 V = normalize(ubo.camPos.xyz - WorldPos);
	vec3 L = normalize(ubo.lightPos.xyz - WorldPos);
	vec3 H = normalize(V + L);
	float distance = length(ubo.lightPos.xyz - WorldPos);
    float attenuation = 1.0;
    vec3 radiance = vec3(1,0,0) * attenuation;			///// LIGHT COLOR

	// Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, H, roughness);
    float G   = GeometrySmith(normal, V, L, roughness);      
    vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

	vec3 nominator    = NDF * G * F;
    float denominator = 4 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0);
    vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;
	float NdotL = max(dot(normal, L), 0.0);
	Lo += (kD * diffuse / PI + specular) * radiance * NdotL;

	vec3 ambient = vec3(0.03) * diffuse;
	vec3 color = ambient + Lo;
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2)); 
    vec4 FragColor = vec4(color, 1.0);

	rayPayload.color = FragColor.xyz * v0.color;
	
	rayPayload.distance = gl_RayTmaxNV;
	rayPayload.normal = normal;

	// Objects with full white vertex color are treated as reflectors
	rayPayload.reflector = ((v0.color.r == 1.0f) && (v0.color.g == 1.0f) && (v0.color.b == 1.0f)) ? 1.0f : 0.0f; 
}
