#version 430 core                                                      
                                                                       
struct Particle                                                        
{                                                                      
    vec3 position;                                                     
    vec3 velocity;                                                     
    float lifeTime;                                                    
};                                                                     
                                                                       
layout(std140, binding=0) buffer Particles                             
{                                                                      
     Particle particles[1000];                                         
};                                                                     
                                                                       
layout(location = 0) uniform mat4 mv;                                  
                                                                       
out vec4 particlePosition;                                             
out float particleSize;                                                
out float particleAlpha;                                               
                                                                       
void main(void)                                                        
{                                                                      
    particlePosition = mv * vec4(particles[gl_VertexID].position, 1.0);
    float lifeTime = particles[gl_VertexID].lifeTime;                  
    particleSize = 0.1 + lifeTime * 0.02;                             
    particleAlpha = pow((10.0 - lifeTime) * 0.1, 7.0) * 0.7;           
}