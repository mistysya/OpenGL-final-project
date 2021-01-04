#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D noiseTexture;
uniform sampler2D blurScene;
uniform int effectID;
uniform float time;
uniform vec2 circlePos;
uniform float circleRadius;
uniform float zoomInRatio;

void main()
{
	if (effectID == 0) // default
    	FragColor = vec4(texture(screenTexture, TexCoords).rgb, 1.0);
	else if (effectID == 1) // image abstraction
	{
        // average blur
		vec4 color = vec4(0, 0, 0, 0);
		int n = 0;
		int half_size = 5;
		for (int i = -half_size; i <= half_size; ++i){
			for (int j = -half_size; j <= half_size; ++j){
				vec4 c = vec4(texture(screenTexture, TexCoords + vec2(i, j) / vec2(800, 600)).rgb, 1.0);
				color += c;
			}
		}
		int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
        color /= sample_count;
        // Quantization
        float nbins = 8.0;
        color = floor(color * nbins) / nbins; 
        // Gaussian DOG
        float sigma_e = 2.0f; 
        float sigma_r = 2.8f; 
        float phi = 3.4f; 
        float tau = 0.99f;
        float twoSigmaESquared = 2.0 * sigma_e * sigma_e; 
        float twoSigmaRSquared = 2.0 * sigma_r * sigma_r; 
        int halfWidth = int(ceil( 2.0 * sigma_r ));
        
        vec2 sum = vec2(0.0); 
        vec2 norm = vec2(0.0);
        for ( int i = -halfWidth; i <= halfWidth; ++i ) { 
            for ( int j = -halfWidth; j <= halfWidth; ++j ) { 
                float d = length(vec2(i,j)); 
                vec2 kernel = vec2( exp( -d * d / twoSigmaESquared ), exp( -d * d / twoSigmaRSquared ));
                vec4 c = vec4(texture(screenTexture, TexCoords + vec2(i, j) / vec2(800, 600)).rgb, 1.0);
                vec2 L = vec2(0.299 * c.r + 0.587 * c.g + 0.114 * c.b);
                norm += 2.0 * kernel; 
                sum += kernel * L; 
            }
        }
        sum /= norm; 
        float H = 100.0 * (sum.x - tau * sum.y); 
        float edge =( H > 0.0 ) ? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H ); 
        color *= vec4(edge, edge, edge, 1.0 );
		FragColor = color;
	}
	else if (effectID == 2) // watercolor
    {
        // average blur
		vec4 color = vec4(0, 0, 0, 0);
		int n = 0;
		int half_size = 5;
		for (int i = -half_size; i <= half_size; ++i){
			for (int j = -half_size; j <= half_size; ++j){
				vec4 c = vec4(texture(screenTexture, TexCoords + vec2(i, j) / vec2(800, 600)).rgb, 1.0);
				color += c;
			}
		}
		int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
        color /= sample_count;
        // noise texture
		vec4 noise = vec4(0, 0, 0, 0);
		n = 0;
		half_size = 4;
		for (int i = -half_size; i <= half_size; ++i){
			for (int j = -half_size; j <= half_size; ++j){
				vec4 noi = vec4(texture(noiseTexture, TexCoords + vec2(i, j) / vec2(800, 600)).rgb, 1.0);
				noise += noi;
			}
		}
		sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
        noise /= sample_count;
        // mixture
        float mixValue = 0.9;
        color = color * mixValue + noise * (1 - mixValue);
        // Quantization
        float nbins = 8.0;
        color = floor(color * nbins) / nbins;
        FragColor = color;
    }
	else if (effectID == 3) // bloom effect
    {
        // Origin
        vec4 origin = vec4(texture(screenTexture, TexCoords).rgb, 1.0);
        // First blur
        vec4 firstBlur = vec4(texture(blurScene, TexCoords).rgb, 1.0);
        // Second blur
		vec4 secondBlur = vec4(0, 0, 0, 0);
		int n = 0;
		int half_size = 5;
		for (int i = -half_size; i <= half_size; ++i){
			for (int j = -half_size; j <= half_size; ++j){
				vec4 c = vec4(texture(blurScene, TexCoords + vec2(i, j) / vec2(800, 600)).rgb, 1.0);
				secondBlur += c;
			}
		}
		int sample_count = (half_size * 2 + 1) * (half_size * 2 + 1);
        secondBlur /= sample_count;
        // output
        float origin_weight = 0.5;
        float first_weight = 0.3;
        float second_weight = 0.2;
        FragColor = origin * origin_weight + firstBlur * first_weight + secondBlur * second_weight;
    }
	else if (effectID == 4) // pixelation
    {
        float weight = 800;
        float height = 600;
        float dx = 15.0 * (1.0 / weight);
        float dy = 10.0 * (1.0 / height);
        vec2 coord = vec2(dx * floor(TexCoords.x / dx),
                          dy * floor(TexCoords.y / dy));
        FragColor = vec4(texture(screenTexture, coord).rgb, 1.0);
    }
	else if (effectID == 5) // sine wave distortion
    {
        float pi = 3.14159;
        float alpha = 0.3;
        float beta = 2.0;
        vec2 coord = TexCoords;
        coord.x += alpha * sin(coord.y * pi * beta + time);
        FragColor = vec4(texture(screenTexture, coord).rgb, 1.0);
    }
	else if (effectID == 6) // magnifier
	{
        vec2 center = vec2(circlePos.x / 800, (600 - circlePos.y) / 600);
        vec2 vectorToCenter = vec2(TexCoords.x - center.x, TexCoords.y - center.y);
        float centerDistance = vectorToCenter.x * vectorToCenter.x + vectorToCenter.y * vectorToCenter.y;
        
        if (centerDistance > circleRadius / 800 * circleRadius / 600) // out of radius
            FragColor = vec4(texture(screenTexture, TexCoords).rgb, 1.0);
        else
        {
            vec2 zoomPos = vec2(vectorToCenter.x / zoomInRatio + center.x, vectorToCenter.y / zoomInRatio + center.y);
            FragColor = vec4(texture(screenTexture, zoomPos).rgb, 1.0);
        }
    }
}
