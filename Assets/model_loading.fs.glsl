#version 420 core

const int NUM_CSM = 3;

out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 N;
in vec3 L;
in vec3 H;
in vec4 shadow_coord;
in vec3 tangentLightPos;
in vec3 tangentViewPos;
in vec3 tangentFragPos;
in vec3 csmPos_W;
in vec4 csmPos_L[NUM_CSM];
in float csmPos_C;
in vec4 CSM_coord[NUM_CSM];

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1; // NOTICE texture index
uniform sampler2DShadow shadow_tex; // NOTICE shadow tex index
uniform sampler2DShadow uDepthTexture[NUM_CSM]; // NOTICE shadow tex index
uniform bool using_normal_color;
uniform bool display_normal_mapping;
uniform bool enable_cascade_shadow;

// Material properties
uniform vec3 diffuse_albedo = vec3(0.35);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 200.0;
uniform float uCascadedRange_C[NUM_CSM];

// Position of light and eyes
uniform vec3 light_pos;
uniform vec3 eye_pos;
/*
float check_shadow(int cascaded_idx, vec4 curr_pos, vec3 N, vec3 L)
{
	vec3 proj_coord = curr_pos.xyz / curr_pos.w;
	if (proj_coord.z > 1.f) {
		return 0.f;
	}
	proj_coord = proj_coord * 0.5f + 0.5f;

	// from camera view
	float curr_depth = proj_coord.z;
	// from light view
	float occ_depth = texture(uDepthTexture[cascaded_idx], proj_coord.xy).x;

	// adaptive bias
	float bias = 0.005f * tan(acos(dot(N, L)));
	bias = clamp(bias, 0.f, 0.01f);

	//if (occ_depth < curr_depth - bias) {
	if (occ_depth < curr_depth + 0.00001) {
		return 0.5f;
	}
	else {
		return 1.0f;
	}
}
*/
void main()
{    
	// Normal Mapping
	vec3 normal = texture(texture_normal1, TexCoords).rgb;
	normal = normalize(normal * 2.0 - 1.0);
	vec3 lightDir = normalize(tangentLightPos - tangentFragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    // Normalize the incoming vectors
    vec3 N = normalize(N);
    vec3 L = normalize(L);
    vec3 H = normalize(H);

    // Compute the diffuse and specular components for each	fragment
    vec3 ambient = vec3(0.3);
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;

	// Test Normal Mapping effect
	if (display_normal_mapping) {
		diffuse = 0.9 * diffuse + 0.1 * vec3(1.0) * diff;
		specular = 0.7 * specular + 0.2 * vec3(0.3) * spec;
	}

    float shadow_factor = textureProj(shadow_tex, shadow_coord) * 0.6 + 0.4;

	// Cascade shadow
	float cascade_shadow_factor = 0;
	vec4 cascaded_indicator = vec4(0.f);

	// choose level of detail for CSM in clip space
	for (int i = 0; i < NUM_CSM; ++i) {
		if (csmPos_C <= uCascadedRange_C[i]) {
			//cascade_shadow_factor = check_shadow(i, csmPos_L[i], N, L) * 0.8 + 0.2;
			cascade_shadow_factor = textureProj(uDepthTexture[i], CSM_coord[i]) * 0.8 + 0.2;

			// visualize frustum sections
			if (i == 0) {
				cascaded_indicator = vec4(0.f, 0.f, 0.f, 0.f);
			}
			else if (i == 1) {
				cascaded_indicator = vec4(0.f, 0.07f, 0.f, 0.f);
			}
			else if (i == 2) {
				cascaded_indicator = vec4(0.f, 0.f, 0.07f, 0.f);
			}

			break;
		}
	}
	if (enable_cascade_shadow)
		shadow_factor = cascade_shadow_factor;

    if (using_normal_color)
        FragColor = vec4(Normal, 1.0f);
    else {
        //FragColor = texture(texture_diffuse1, TexCoords) * vec4(ambient + diffuse + specular, 1.0);
        //FragColor = vec4(shadow_factor, shadow_factor, shadow_factor, 1.0);
        FragColor = texture(texture_diffuse1, TexCoords) * vec4(shadow_factor * (ambient + diffuse + specular), 1.0);

		if (enable_cascade_shadow)
			FragColor = FragColor + cascaded_indicator;
		/*
			else
				FragColor = FragColor;
				*/
        //FragColor = texture(texture_diffuse1, TexCoords) * vec4(ambient + diffuse + specular, 1.0);
        //FragColor = vec4(ambient + diffuse + specular, 1.0);
    }
}