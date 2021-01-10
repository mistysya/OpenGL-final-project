#version 420 core

in vec4 clipSpace;

in vec2 textureCoords;

out vec4 FragColor;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;

void main()
{
	vec2 ndc = (clipSpace.xy / clipSpace.w) / 2.0 + 0.5;
	vec2 reflectTexCoords = vec2(ndc.x, -ndc.y);
	vec2 refractTexCoords = vec2(ndc.x, ndc.y);

	//vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
	//vec4 refractColor = texture(refractionTexture, refractTexCoords);
	vec4 reflectColor = texture(reflectionTexture, textureCoords);
	vec4 refractColor = texture(refractionTexture, textureCoords);

	//FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
	FragColor = mix(reflectColor, refractColor, 0.5);
	//FragColor = reflectColor;
}