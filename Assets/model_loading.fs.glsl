#version 420 core
out vec4 FragColor;

const int NUM_CSM = 3;

in vec3 Normal;
in vec2 TexCoords;
in vec3 N;
in vec3 L;
in vec3 H;
in vec4 shadow_coord;
in vec4 CSM_coord[NUM_CSM];
in float depth_current_position;

uniform sampler2D texture_diffuse1;
uniform sampler2DShadow shadow_tex;
uniform sampler2DShadow shadow_texes[NUM_CSM];
uniform bool using_normal_color;

// CSM
uniform float uCascadedRange_C[NUM_CSM];

// Material properties
uniform vec3 diffuse_albedo = vec3(0.35);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 200.0;

// shadow factor
float shadow_factor;

void main()
{    
    // Normalize the incoming vectors
    vec3 N = normalize(N);
    vec3 L = normalize(L);
    vec3 H = normalize(H);

    // Compute the diffuse and specular components for each	fragment
    vec3 ambient = vec3(0.3);
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;

    // CSM
    for (int i = 0; i < NUM_CSM; ++i) {
        if (depth_current_position <= uCascadedRange_C[i]) {
            shadow_factor = textureProj(shadow_texes[i], CSM_coord[i]) * 0.8 + 0.2;
        }
    }
    // SM
    //shadow_factor = textureProj(shadow_tex, shadow_coord) * 0.8 + 0.2;

    if (using_normal_color)
        FragColor = vec4(Normal, 1.0f);
    else {
        //FragColor = texture(texture_diffuse1, TexCoords) * vec4(ambient + diffuse + specular, 1.0);
        //FragColor = vec4(shadow_factor, shadow_factor, shadow_factor, 1.0);
        FragColor = texture(texture_diffuse1, TexCoords) * vec4(shadow_factor * (ambient + diffuse + specular), 1.0);
        //FragColor = texture(texture_diffuse1, TexCoords) * vec4(ambient + diffuse + specular, 1.0);
        //FragColor = vec4(ambient + diffuse + specular, 1.0);
    }
}