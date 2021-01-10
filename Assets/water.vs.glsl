#version 420 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 textureCoords;
out vec4 clipSpace;

// Position of eyes
uniform vec3 eye_pos;
uniform vec3 light_pos;


void main()
{
	clipSpace = projection * view * model * vec4(aPos.x, 0.0, aPos.y, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
	textureCoords = vec2(aPos.x / 2.0 + 0.5, aPos.y / 2.0 + 0.5);
	//gl_Position = clipSpace;
}