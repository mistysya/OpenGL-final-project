#version 430 core

out vec4 FragColor;

uniform samplerCube tex_cubemap;

in vec3 N_ENV;
in vec3 rippleNormal;
in vec3 rippleView;

// Material properties
const vec3 waterColor = vec3(0.55, 0.8, 0.95);
const float ambient = 0.2;
const float shininess = 200.0;

// Light vector
uniform vec3 light_pos;

void main()
{
    // Normalize the incoming vectors
	vec3 V = normalize(rippleView);
    vec3 N = normalize(rippleNormal);
    vec3 L = normalize(light_pos);
    vec3 H = normalize(L + V);

    // Compute the diffuse and specular components for each	fragment
	vec3 diffuse = waterColor * (ambient + vec3(0.35) * max(0, dot(N, L)));
	vec3 spec = vec3(0.7) * pow(max(0, dot(H, N)), shininess);

    // Reflect view vector about the plane defined by the normal at the fragment
	vec3 rfl = texture(tex_cubemap, reflect(-V, normalize(N_ENV))).rgb;
	vec3 rfr = texture(tex_cubemap, refract(-V, normalize(N_ENV), 1 / 1.33)).rgb;

	vec3 env = rfl * 0.5 + rfr * 0.5;

	vec3 color = env * 0.5 + (diffuse + spec) * 0.5;

    FragColor = vec4(color, 1.0);
}