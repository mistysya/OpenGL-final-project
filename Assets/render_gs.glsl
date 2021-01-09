#version 430 core                                                               
                                                                                
layout(points, invocations = 1) in;                                             
layout(triangle_strip, max_vertices = 4) out;                                   
                                                                                
layout(location = 1) uniform mat4 p;                                            
                                                                                
in vec4 particlePosition[];                                                     
in float particleSize[];                                                        
in float particleAlpha[];                                                       
                                                                                
out float particleAlphaOut;                                                     
out vec2 texcoord;                                                              
                                                                                
void main(void)                                                                 
{                                                                               
    vec4 verts[4];                                                              
    verts[0] = p * (particlePosition[0] + vec4(-1, -1, 0, 0) * particleSize[0]);
    verts[1] = p * (particlePosition[0] + vec4(1, -1, 0, 0) * particleSize[0]); 
    verts[2] = p * (particlePosition[0] + vec4(1, 1, 0, 0) * particleSize[0]);  
    verts[3] = p * (particlePosition[0] + vec4(-1, 1, 0, 0) * particleSize[0]); 
                                                                                
    gl_Position = verts[0];                                                     
    particleAlphaOut = particleAlpha[0];                                        
    texcoord = vec2(0, 0);                                                      
    EmitVertex();                                                               
                                                                                
    gl_Position = verts[1];                                                     
    particleAlphaOut = particleAlpha[0];                                        
    texcoord = vec2(1, 0);                                                      
    EmitVertex();                                                               
                                                                                
    gl_Position = verts[3];                                                     
    particleAlphaOut = particleAlpha[0];                                        
    texcoord = vec2(0, 1);                                                      
    EmitVertex();                                                               
                                                                                
    gl_Position = verts[2];                                                     
    particleAlphaOut = particleAlpha[0];                                        
    texcoord = vec2(1, 1);                                                      
    EmitVertex();                                                               
                                                                                
    EndPrimitive();                                                             
}