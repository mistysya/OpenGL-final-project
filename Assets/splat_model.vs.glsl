#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 N;
out vec3 L;
out vec3 H;
out vec3 N_ENV;
out vec3 ENV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Position of light and eyes
uniform vec3 light_pos;
uniform vec3 eye_pos;

void main()
{
    Normal = aNormal;
    TexCoords = aTexCoords; 
	
	vec4 P = view * model * vec4(aPos, 1.0);
	vec4 W = model * vec4(aPos, 1.0);

	// Calculate Bling-Phong vector
	N = mat3(view * model) * aNormal;
	L = light_pos;
	H = light_pos - P.xyz;

	// Calculate ENV view vector
	N_ENV = mat3(transpose(inverse(model))) * aNormal;
	ENV = W.xyz - eye_pos;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}