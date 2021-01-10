#version 410 core                                                                                           
                                                                                                            
in vec4 vVertex;

out vec4 color;  

uniform mat4 mvpMatrix;    

uniform int Time;
                                                                                                            
int random(int seed, int iterations) 
{
    int value = seed;                                                                                       
    int n;                                                                                                                                                                                                         
    for (n = 0; n < iterations; n++) {                                                                      
        value = ((value >> 7) ^ (value << 9)) * 15485863;                                                   
    }                                                                                                       
                                                                                                            
    return value;                                                                                           
}                                                                                                                                                                                                                       
vec4 random_vector(int seed)                                                                                
{                                                                                                           
    int r = random(gl_InstanceID, 4);                                                                       
    int g = random(r, 2);                                                                                   
    int b = random(g, 2);                                                                                   
    int a = random(b, 2);                                                                                   
                                                                                                            
    return vec4(float(r & 0x3FF) / 1024.0,                                                                  
                float(g & 0x3FF) / 1024.0,                                                                  
                float(b & 0x3FF) / 1024.0,                                                                  
                float(a & 0x3FF) / 1024.0);                                                                 
}                                                                                                           
                                                                                                            
mat4 construct_rotation_matrix(float angle)                                                                 
{                                                                                                           
    float st = sin(angle);                                                                                  
    float ct = cos(angle);                                                                                  
                                                                                                            
    return mat4(vec4(ct, 0.0, st, 0.0),                                                                     
                vec4(0.0, 1.0, 0.0, 0.0),                                                                   
                vec4(-st, 0.0, ct, 0.0),                                                                    
                vec4(0.0, 0.0, 0.0, 1.0));                                                                  
}         

mat4 rotationMatrix(float angle)
{
    float st = sin(angle);
    float ct = cos(angle);

    return mat4(vec4(ct, st,0.0 , 0.0),
                vec4(-st, ct, 0.0, 0.0),
                vec4(0.0, 0.0, 1.0, 0.0),
                vec4(0.0, 0.0, 0.0, 1.0));
}
                                                                                                            
void main(void)                                                                                             
{                                                                                                           
    vec4 offset = vec4(float(gl_InstanceID >> 10) - 512.0,                                                  
                       0.0f,                                                                                
                       float(gl_InstanceID & 0x3FF) - 512.0,                                                
                       0.0f);                                                                               
    int number1 = random(gl_InstanceID, 3);                                                                 
    int number2 = random(number1, 2);                                                                       
    offset += vec4(float(number1 & 0xFF) / 256.0,                                                           
                   0.0f,                                                                                    
                   float(number2 & 0xFF) / 256.0,                                                           
                   0.0f);                                                                                   
    float angle = float(random(number2, 2) & 0x3FF) / 1024.0;                                               
                                                                                                            
    vec2 texcoord = offset.xz / 1024.0 + vec2(0.5);                                                         
                                                                                                            
    float bend_factor = float(random(number2, 7) & 0x3FF) / 1024.0;                                         
    float bend_amount = cos(vVertex.y);                                                                     
                                                                                                            
    mat4 rot = construct_rotation_matrix(angle);     
    mat4 sine_rot = rotationMatrix(sin(float(Time+ offset*0.5) * 0.07f) * 0.5f);
    vec4 position = (sine_rot*rot * (vVertex + vec4(0.0, 0.0, bend_amount * bend_factor, 0.0))) + offset;
                                                                                                            
    position *= vec4(1.0, float(number1 & 0xFF) / 256.0 * 0.9 + 0.3, 1.0, 1.0);                             
    gl_Position = mvpMatrix * position;                                                                     
    color = vec4(random_vector(gl_InstanceID).xyz * vec3(0.1, 0.5, 0.1) + vec3(0.1, 0.4, 0.1), 1.0);        
}