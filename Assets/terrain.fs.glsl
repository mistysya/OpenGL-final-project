#version 420 core

out vec4 color;

const int NUM_CSM = 3;

uniform sampler2D tex_color;
uniform sampler2DShadow shadow_tex;
// shadow
uniform mat4 shadow_matrix;


// CSM
uniform sampler2DShadow shadow_texes[NUM_CSM];
uniform mat4 shadow_matrices[NUM_CSM];
uniform float uCascadedRange_C[NUM_CSM];

// shadow factor
float shadow_factor;
vec4 cascaded_indicator = vec4(0.f);

uniform bool enable_fog = true;
uniform vec4 fog_color = vec4(0.7, 0.8, 0.9, 0.0);

uniform bool use_cascade;

in TES_OUT
{
    vec2 tc;
    vec3 world_coord;
    vec3 eye_coord;
} fs_in;

in float depth_current_position;

vec4 fog(vec4 c)
{
    float z = length(fs_in.eye_coord);

    float de = 0.025 * smoothstep(0.0, 6.0, 10.0 - fs_in.world_coord.y);
    float di = 0.045 * (smoothstep(0.0, 40.0, 20.0 - fs_in.world_coord.y));

    float extinction   = exp(-z * de);
    float inscattering = exp(-z * di);

    return c * extinction + fog_color * (1.0 - inscattering);
}

void main(void)
{
    vec4 landscape = texture(tex_color, fs_in.tc);
	vec4 shadow_coord = shadow_matrix * vec4(fs_in.world_coord.xyz, 1.0);
	vec4 CSM_coord[NUM_CSM];

	for (int i = 0; i < NUM_CSM; ++i) {
		CSM_coord[i] = shadow_matrices[i] * vec4(fs_in.world_coord, 1.0);
	}

	// CSM
	if (use_cascade)
	{
		for (int i = 0; i < NUM_CSM; ++i) {
			if (depth_current_position <= uCascadedRange_C[i]) {
				shadow_factor = textureProj(shadow_texes[i], CSM_coord[i]) * 0.8 + 0.2;

				// visualize frustum sections
				if (i == 0) {
					cascaded_indicator = vec4(0.1f, 0.f, 0.f, 0.f);
				}
				else if (i == 1) {
					cascaded_indicator = vec4(0.f, 0.1f, 0.f, 0.f);
				}
				else if (i == 2) {
					cascaded_indicator = vec4(0.f, 0.f, 0.1f, 0.f);
				}

				break;
			}
		}
	}
	else
	{
		// SM
		shadow_factor = textureProj(shadow_tex, shadow_coord) * 0.8 + 0.2;
	}
	landscape = landscape * vec4(vec3(shadow_factor), 1.0);

    if (enable_fog)
    {
        color = fog(landscape);
    }
    else
    {
        color = landscape;
    }
}     