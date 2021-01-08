#version 430 core                                                 
                                                                  
layout(binding = 0) uniform sampler2D particleTex;                
                                                                  
layout(location = 0) out vec4 fragColor;                          
                                                                  
in float particleAlphaOut;                                        
in vec2 texcoord;                                                 
                                                                  
void main(void)                                                   
{                                                                 
    fragColor = texture(particleTex, texcoord) * particleAlphaOut;
}