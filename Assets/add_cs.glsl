#version 430 core                                                                                               
                                                                                                                
layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;                                             
                                                                                                                
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
                                                                                                                
layout(binding = 0, offset = 0) uniform atomic_uint count;                                                      
layout(location = 0) uniform uint addCount;                                                                     
layout(location = 1) uniform vec2 randomSeed;                                                                   
                                                                                                                
float rand(vec2 n)                                                                                              
{                                                                                                               
    return fract(sin(dot(n.xy, vec2(12.9898, 78.233))) * 43758.5453);                                           
}                                                                                                               
                                                                                                                
const float PI2 = 6.28318530718;                                                                                
                                                                                                                
void main(void)                                                                                                 
{                                                                                                               
    if(gl_GlobalInvocationID.x < addCount)                                                                      
    {                                                                                                           
        uint idx = atomicCounterIncrement(count);                                                               
        float rand1 = rand(randomSeed + vec2(float(gl_GlobalInvocationID.x * 2)));                              
        float rand2 = rand(randomSeed + vec2(float(gl_GlobalInvocationID.x * 2 + 1)));                          
        particles[idx].position = vec3(0, 0, 0);                                                                
        particles[idx].velocity = normalize(vec3(cos(rand1 * PI2), 5.0 + rand2 * 5.0, sin(rand1 * PI2))) * 0.15;
        particles[idx].lifeTime = 0;                                                                            
    }                                                                                                           
}                                                                             