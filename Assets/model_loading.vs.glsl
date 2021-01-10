#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

const int NUM_CSM = 3;

out vec2 TexCoords;
out vec3 Normal;
out vec3 N;
out vec3 L;
out vec3 H;
out vec4 shadow_coord;
out vec4 CSM_coord[NUM_CSM];
out float depth_current_position;
out vec3 tangentLightPos;
out vec3 tangentViewPos;
out vec3 tangentFragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Position of light and eyes
uniform vec3 light_pos;
uniform vec3 eye_pos;

// shadow
uniform mat4 shadow_matrix;
// CSM
uniform mat4 shadow_matrices[NUM_CSM];

// water
uniform vec3 plane;
uniform float plane_height;

void main()
{
	vec4 plane_clip = vec4(plane, plane_height);
	gl_ClipDistance[0] = dot(model * vec4(aPos, 1.0), plane_clip);
    Normal = aNormal;
    TexCoords = aTexCoords; 
	
	vec4 P = view * model * vec4(aPos, 1.0);
	N = mat3(view * model) * aNormal;
	L = light_pos;
	H = light_pos - P.xyz;

	// Normal mapping
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vec3 T = normalize(normalMatrix * aTangent);
	vec3 N = normalize(normalMatrix * aNormal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = transpose(mat3(T, B, N));
	tangentLightPos = TBN * light_pos;
	tangentViewPos = TBN * eye_pos;
	tangentFragPos = TBN * vec3(model * vec4(aPos, 1.0));

	// shadow
	shadow_coord = shadow_matrix * vec4(aPos, 1.0);

	// CSM
	for (int i = 0; i < NUM_CSM; ++i) {
		CSM_coord[i] = shadow_matrices[i] * vec4(aPos, 1.0);
	}

    gl_Position = projection * view * model * vec4(aPos, 1.0);

	depth_current_position = gl_Position.z;
}