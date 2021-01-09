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
out vec3 tangentLightPos;
out vec3 tangentViewPos;
out vec3 tangentFragPos;
out vec3 csmPos_W;
out vec4 csmPos_L[NUM_CSM];
out float csmPos_C;
out vec4 CSM_coord[NUM_CSM];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 csm_L[NUM_CSM];
uniform mat4 shadow_matrices[NUM_CSM];

// Position of light and eyes
uniform vec3 light_pos;
uniform vec3 eye_pos;

// shadow
uniform mat4 shadow_matrix;

void main()
{
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

    gl_Position = projection * view * model * vec4(aPos, 1.0);

	// Cascade shadow
	for (int i = 0; i < NUM_CSM; ++i) {
		csmPos_L[i] = csm_L[i] * model * vec4(aPos, 1.0);
		CSM_coord[i] = shadow_matrices[i] * vec4(aPos, 1.0);
	}
	csmPos_C = gl_Position.z;
	csmPos_W = vec3(model * vec4(aPos, 1.0));

	// shadow
	shadow_coord = shadow_matrix * vec4(aPos, 1.0);
}