#version 430 core

out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 N;
in vec3 L;
in vec3 H;
in vec3 N_ENV;
in vec3 ENV;

uniform samplerCube tex_cubemap;
uniform sampler2D texture_diffuse1;
uniform bool using_normal_color;

// Material properties
const vec4 splatColor = vec4(0.55, 0.8, 0.95, 1.0);
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

    // Reflect view vector about the plane defined by the normal at the fragment
    // I = -L !!!
    vec3 R = reflect(normalize(ENV), normalize(N_ENV));

    if (using_normal_color)
        FragColor = vec4(Normal, 1.0f);
    else
        FragColor = (splatColor * vec4(ambient + diffuse + specular, 1.0)) * 0.6 + texture(tex_cubemap, R) * 0.4;
}