#version 330 core

// Input vertex data
layout (location = 0) in vec3 position;   
layout (location = 1) in vec3 normal;     
layout (location = 2) in vec2 texCoord;   

out vec2 tc;  

uniform mat4 mv_matrix;    
uniform mat4 proj_matrix;
uniform float modelScale;  

void main() {
    vec3 scaledPosition = position * modelScale;
    gl_Position = proj_matrix * mv_matrix * vec4(scaledPosition, 1.0);
    tc = texCoord;
}