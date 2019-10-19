#version 330

out vec4 FragColor;

in vec2 TexCoord0;
in vec3 Normal0;                                                                   
in vec3 WorldPos0;                                                                 
                                                                         
uniform sampler2D texture_diffuse1;
                                                                                            
void main()
{                                    
    FragColor = texture(texture_diffuse1, TexCoord0.xy);
}