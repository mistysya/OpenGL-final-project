#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool using_normal_color;

void main()
{    
    if (using_normal_color)
        FragColor = vec4(Normal, 1.0f);
    else
        FragColor = texture(texture_diffuse1, TexCoords);
}