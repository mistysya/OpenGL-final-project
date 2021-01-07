#version 420 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 N;
in vec3 L;
in vec3 H;
in vec4 shadow_coord;

uniform sampler2D texture_diffuse1;
uniform sampler2DShadow shadow_tex;
uniform bool using_normal_color;

// Material properties
uniform vec3 diffuse_albedo = vec3(0.35);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 200.0;

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
    float shadow_factor = textureProj(shadow_tex, shadow_coord) * 0.6 + 0.4;

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