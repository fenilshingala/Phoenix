#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;
in vec3 BaseColor;

void main()
{		
    FragColor = vec4(BaseColor, 1.0);
}