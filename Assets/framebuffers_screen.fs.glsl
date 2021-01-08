#version 420 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D ssaoTexture;

uniform bool ssao;

void main()
{
	vec4 OriColor = vec4(texture(screenTexture, TexCoords).rgb, 1.0);
	if(ssao)
		FragColor = texture(ssaoTexture, TexCoords) * OriColor;
	else
	    FragColor = OriColor;
    //vec3 col = texture(screenTexture, TexCoords).rgb;
    //FragColor = vec4(col, 1.0);
	//FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);
} 