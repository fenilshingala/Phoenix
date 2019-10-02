#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in mat4 aModel;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform bool notInstanced;

void main()
{
	mat4 myModel = aModel;
	if(notInstanced)
	{
		myModel = model;
	}

    vec4 worldPos = myModel * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(myModel)));
    Normal = normalMatrix * aNormal;

    gl_Position = projection * view * worldPos;
}