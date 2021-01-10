#version 420 core

out VS_OUT{ vec2 tc;} vs_out;

out float depth_current_position;

void main()
{
    const vec4 vertices[] = vec4[](vec4(-0.5, 0.0, -0.5, 1.0),
								   vec4( 0.5, 0.0, -0.5, 1.0),
								   vec4(-0.5, 0.0,  0.5, 1.0),
								   vec4( 0.5, 0.0,  0.5, 1.0));
	int x = gl_InstanceID & 63;
	int y = gl_InstanceID >> 6;
	vec2 offs = vec2(x, y);

	vs_out.tc = (vertices[gl_VertexID].xz + offs + vec2(0.5)) / 64.0;
	gl_Position = vertices[gl_VertexID] + vec4(float(x - 32), 0.0,float(y - 32), 0.0);

	depth_current_position = gl_Position.z;
}
