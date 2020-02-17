#version 330 core
out vec4 FragColor;

//in vec3 Normal;
//in vec3 FragPos;
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec4 FragPosLightSpace;
} fs_in;

//uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
 
	projCoords = projCoords * 0.5 + 0.5;
	
	float closestDepth = texture(shadowMap, projCoords.xy).r; 
    
    float currentDepth = projCoords.z;
    
	float bias = 0.005;
    
	//float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
	
	// PERCENTAGE CLOSER FILTERING
	float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

	// keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
  	
    // diffuse 
    vec3 norm = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

	float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
        
    vec3 result = (ambient + (1.0 - shadow) * (diffuse + specular)) * objectColor;
    FragColor = vec4(result, 1.0);
}