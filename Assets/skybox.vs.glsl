#version 420

out VS_OUT
{
	vec3 tc;
} vs_out;

uniform mat4 pv_matrix;
uniform vec3 cam_pos;

void main(void)
{
	vec3[4] vertices = vec3[4](vec3(-1.0, -1.0, 1.0),
		vec3(1.0, -1.0, 1.0),
		vec3(-1.0, 1.0, 1.0),
		vec3(1.0, 1.0, 1.0));

	vec4 tc = inverse(pv_matrix) * vec4(vertices[gl_VertexID], 1.0);
	vs_out.tc = tc.xyz / tc.w - cam_pos;
	gl_Position = vec4(vertices[gl_VertexID], 1.0);

}