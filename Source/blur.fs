#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    vec4 fistBlur = vec4(0, 0, 0, 0);
	int n = 0;
	int half_size = 5;
	for (int i = -half_size; i <= half_size; ++i){
		for (int j = -half_size; j <= half_size; ++j){
			vec4 c = vec4(texture(screenTexture, TexCoords + vec2(i, j) / vec2(800, 600)).rgb, 1.0);
			fistBlur += c;
		}
	}
	int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
    fistBlur /= sample_count;
    FragColor = fistBlur;
}