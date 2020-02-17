#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 aModel;
layout (location = 7) in vec3 aColor;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 Normal;
out vec3 BaseColor;

uniform vec3 color;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int instanced;

void main()
{
	mat4 myModel = model;
	BaseColor = color;

	if(instanced == 1)
	{
		myModel = aModel;
		BaseColor = aColor;
	}

    WorldPos = vec3(myModel * vec4(aPos, 1.0));
    TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(myModel)));
    Normal = normalMatrix * aNormal;

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}