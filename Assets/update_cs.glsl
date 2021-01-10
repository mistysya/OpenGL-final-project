#version 430 core                                                                                               
                                                                                                                
layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;                                             
                                                                                                                
struct Particle                                                                                                 
{                                                                                                               
    vec3 position;                                                                                              
    vec3 velocity;                                                                                              
    float lifeTime;                                                                                             
};                                                                                                              
                                                                                                                
layout(std140, binding=0) buffer InParticles                                                                    
{                                                                                                               
     Particle inParticles[1000];                                                                                
};                                                                                                              
                                                                                                                
layout(std140, binding=1) buffer OutParticles                                                                   
{                                                                                                               
     Particle outParticles[1000];                                                                               
};                                                                                                              
                                                                                                                
layout(binding = 0, offset = 0) uniform atomic_uint inCount;                                                    
layout(binding = 1, offset = 0) uniform atomic_uint outCount;                                                   
layout(location = 0) uniform float deltaTime;                                                                   
                                                                                                                
const vec3 windAccel = vec3(0, 0, 0);                                                                       
                                                                                                                
void main(void)                                                                                                 
{                                                                                                               
    uint idx = gl_GlobalInvocationID.x;                                                                         
    if(idx < atomicCounter(inCount))                                                                            
    {                                                                                                           
        float lifeTime = inParticles[idx].lifeTime + deltaTime;                                                 
        if(lifeTime < 5.0)                                                                                     
        {                                                                                                       
            uint outIdx = atomicCounterIncrement(outCount);                                                     
            outParticles[outIdx].position = inParticles[idx].position + inParticles[idx].velocity * deltaTime;  
            outParticles[outIdx].velocity = inParticles[idx].velocity + windAccel * deltaTime;                  
            outParticles[outIdx].lifeTime = lifeTime;                                                           
        }                                                                                                       
    }                                                                                                           
}