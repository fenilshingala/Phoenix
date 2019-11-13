#version 330 core

out vec4 FragColor;

void main()
{             
	float depth = gl_FragCoord.z;
	vec4 momentDepth = vec4(depth, depth*depth, depth*depth*depth, depth*depth*depth*depth);
	
	FragColor = momentDepth;
}