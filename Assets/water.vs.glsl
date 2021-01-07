#version 430 core

struct WaterColumn
{
	float height;
	float flow;
};

layout(std430, binding = 0) buffer WaterGrid
{
	WaterColumn columns[];
};

out vec3 N_ENV;
out vec3 rippleNormal;
out vec3 rippleView;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Position of eyes
uniform vec3 eye_pos;

#define ROW_SIZE 180
const uint offset[4] = uint[4](0, 1, ROW_SIZE, ROW_SIZE + 1);
const vec2 poffset[4] = vec2[4](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(1, 1));

void main()
{
	// For ripples
	// ---------------------
	uvec2 coord = uvec2(mod(gl_InstanceID, ROW_SIZE - 1), uint(gl_InstanceID / (ROW_SIZE - 1)));
	uint idx = coord.y * ROW_SIZE + coord.x;
	idx = idx + offset[gl_VertexID];

	vec3 pos = vec3(float(coord.x) - float(ROW_SIZE / 2) + poffset[gl_VertexID].x, columns[idx].height, float(coord.y) - float(ROW_SIZE / 2) + poffset[gl_VertexID].y);
	// ---------------------

    gl_Position = projection * view * model * vec4(pos, 1.0);

	// For ripples continue
	vec3 dx = vec3(1, 0, 0);
	vec3 dy = vec3(0, 0, 1);
	if (coord.x < ROW_SIZE - 2)
		dx = vec3(1, columns[idx + 1].height - columns[idx].height, 0);
	if (coord.y < ROW_SIZE - 2)
		dy = vec3(0, columns[idx + ROW_SIZE].height - columns[idx].height, 1);
	rippleNormal = normalize(cross(dy, dx));
	rippleView = eye_pos - pos;
	N_ENV = mat3(transpose(inverse(model))) * rippleNormal;
}