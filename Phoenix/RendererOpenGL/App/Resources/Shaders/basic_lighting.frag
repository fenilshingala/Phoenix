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

vec3 Cholesky(float m11, float m12, float m13, float m22, float m23, float m33, float z1, float z2, float z3)
{
	float c1, c2, c3;

	float a = sqrt(m11);
	float b = m12 / a;
	float c = m13 / a;
	float d = sqrt(m22 - b*b);
	float e = (m22 - b*c) / d;
	float f = sqrt(m33 - c*c - e*e);

	float c1_cap = z1 / a;
	float c2_cap = (z2 - b*c1_cap) / d;
	float c3_cap = (z3 - c*c1_cap - e*c2_cap) / f;

	c3 = c3_cap / f;
	c2 = (c2_cap - e*c3) / d;
	c1 = (c1_cap - b*c2 - c*c3) / a;

	return vec3(c1, c2, c3);
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
 
	projCoords = projCoords * 0.5 + 0.5;
	
	float closestDepth = texture(shadowMap, projCoords.xy).r;

    float currentDepth = projCoords.z;

	///////////////////////////////////////////////////////////////////////////
	////////////////////////////// MSM ////////////////////////////////////////

	//float z = texture(shadowMap, projCoords.xy).r;
	float z2 = texture(shadowMap, projCoords.xy).g;
	float z3 = texture(shadowMap, projCoords.xy).b;
	float z4 = texture(shadowMap, projCoords.xy).a;
	
	float alpha = 0.00005; // bias
	vec4 b = vec4(closestDepth, z2, z3, z4);
	vec4 bDash = (1 - alpha) * b + alpha * (0.5, 0.5, 0.5, 0.5);
	mat3 B = mat3(vec3(1, bDash.rg), bDash.rgb, bDash.gba);
	//vec3 c = inverse(B) * vec3(1, b.rg);

	// cholesky's decomposition
	vec3 c = Cholesky(B[0][0], B[0][1], B[0][2], B[1][1], B[1][2], B[2][2], 1, b.r, b.g);

	/////
	float D = sqrt((c.g * c.g) - 4 * c.r * c.b);
	float root1 = (-c.g + D) / 2 * c.b;
	float root2 = (-c.g - D) / 2 * c.r;

	float z_2 = 0.0, z_3 = 0.0;
	if(root1 <= root2)
	{
		z_2 = root1;
		z_3 = root2;
	}
	else // root1 > root2
	{
		z_2 = root2;
		z_3 = root1;
	}

	float G = 0.0;
	if(currentDepth <= z_2)
	{
		G = 0.0;
	}
	else if(currentDepth <= z_3)
	{
		float numerator   = currentDepth * z_3 - (bDash.r * (currentDepth + z_3)) + bDash.g;
		float denominator = (z_3 - z_2) * (currentDepth - z_2);
		G = numerator / denominator;
	}
	else
	{
		float numerator   = z_2 * z_3 - (bDash.r * (z_2 + z_3)) + bDash.g;
		float denominator = (currentDepth - z_2) * (currentDepth - z_3);
		G = 1 - (numerator / denominator);
	}

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
    
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

    //return shadow;
	return G;
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