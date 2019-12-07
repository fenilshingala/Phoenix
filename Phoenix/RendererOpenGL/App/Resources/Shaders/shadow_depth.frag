#version 330 core

layout(location = 0) out vec4 shadowMap;
//layout(location = 1) out vec4 depthMoments;

void main()
{             
	shadowMap.r = gl_FragCoord.z;
	shadowMap.g = gl_FragCoord.z*gl_FragCoord.z;
	shadowMap.b = gl_FragCoord.z*gl_FragCoord.z*gl_FragCoord.z;
	shadowMap.a = gl_FragCoord.z*gl_FragCoord.z*gl_FragCoord.z*gl_FragCoord.z; 
	
	//depthMoments.r = gl_FragCoord.z;
	//depthMoments.g = gl_FragCoord.z*gl_FragCoord.z;
	//depthMoments.b = gl_FragCoord.z*gl_FragCoord.z*gl_FragCoord.z;
	//depthMoments.a = gl_FragCoord.z*gl_FragCoord.z*gl_FragCoord.z*gl_FragCoord.z;    
}