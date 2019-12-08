#version 330 core
out float FragColor;

#define PI 3.1415926535

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;

uniform int kernelSamples;
uniform float radius;

uniform float Width;
uniform float Height;
uniform float scale_factor;
uniform float contrast_factor;

float HeavisideFunction(float a_value)
{
	if(a_value < 0)
	{
		return 0;
	}
	else 
	{
		return 1;
	}
}

void main()
{
	vec3 FragPos = texture(gPosition, TexCoords).rgb; 
	vec3 Normals = texture(gNormal, TexCoords).rgb;
	
	float occlusion = 0.0f;
	vec3 FragPosi = vec3(0);
	float c = 0.1 * radius;
	float delta = 0.001;
	ivec2 xy_dash = ivec2(gl_FragCoord.xy);
	vec2 xy = vec2(xy_dash.x / Width , xy_dash.y / Height);
	
	float d = FragPos.z;
	
	float phi = (30 * xy_dash.x ^ xy_dash.y) + (10 * xy_dash.x * xy_dash.y);
	
	//Visit through samples
	for(int i = 0; i < kernelSamples ; ++i)
	{
		//======================================
		//Fragposi(Wi) calculation
		
		float alpha = ( i+ 0.5) / kernelSamples;
		float h = alpha * radius / d;
		float theta = 2 * PI * alpha * (7 * kernelSamples / 9) + phi;
		
		FragPosi = texture(gPosition, xy + h * vec2(cos(theta),sin(theta))).xyz;
		
		//======================================
		
		vec3 wi = FragPosi - FragPos;
		float H = HeavisideFunction(radius - length(wi));
		float di= FragPosi.z;
		occlusion += max(0,dot(Normals,wi) - delta * di) * H / max(c * c, dot(wi,wi));
		
		
	}
	
	occlusion = ((2 * PI * c) / kernelSamples) * occlusion; 
	
	FragColor = max(pow(1 - scale_factor * occlusion , contrast_factor),0);
}