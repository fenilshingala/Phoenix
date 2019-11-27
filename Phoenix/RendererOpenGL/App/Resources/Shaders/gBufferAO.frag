#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gMaterial;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

//uniform sampler2D texture_diffuse1;
//uniform sampler2D texture_specular1;

// material parameters
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

uniform vec3  u_vAlbedo;
uniform float u_fMetallic;
uniform float u_fRoughness;
uniform float u_fAo;

uniform bool isNotTextured;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{   
    gPosition = WorldPos;
    gNormal = getNormalFromMap();
	gAlbedo.rgb = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));

	float metallic  = texture(metallicMap, TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).r;
    float ao        = texture(aoMap, TexCoords).r;

	if(isNotTextured)
	{
		gNormal		= normalize(Normal);
		gAlbedo.rgb = u_vAlbedo;
		metallic	= u_fMetallic;
		roughness	= u_fRoughness;
		ao			= u_fAo;
	}

	gMaterial		= vec3(metallic, roughness, ao);
}