#version 330                                                                        
                                                                                    
layout (location = 0) in vec3 Position;                                             
layout (location = 1) in vec3 Normal;                                               
layout (location = 2) in vec2 TexCoord;                                             
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;

out vec2 TexCoord0;
out vec3 Normal0;                                                                   
out vec3 WorldPos0;                                                                 

const int MAX_BONES = 100;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 gBones[MAX_BONES];

uniform bool isAnim;

void main()
{       
	vec4 PosL = vec4(0.0, 0.0, 0.0, 0.0);

	mat4 BoneTransform;
	if(isAnim)
	{
		BoneTransform = gBones[BoneIDs[0]] * Weights[0];
		BoneTransform     += gBones[BoneIDs[1]] * Weights[1];
		BoneTransform     += gBones[BoneIDs[2]] * Weights[2];
		BoneTransform     += gBones[BoneIDs[3]] * Weights[3];
		
		PosL = BoneTransform * vec4(Position, 1.0);
	}
	else
	{
		PosL = vec4(Position, 1.0);
	}

    gl_Position  = projection * view * model * PosL;
    TexCoord0    = TexCoord;
	if(isAnim)
	{
		vec4 NormalL = BoneTransform * vec4(Normal, 0.0);
		Normal0      = (model * NormalL).xyz;
	}
	else
	{
		vec4 NormalL = vec4(Normal, 0.0);
		Normal0      = (model * NormalL).xyz;
	}
    WorldPos0    = (model * PosL).xyz;
}