#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 _inFragColor;

layout(location = 0) out vec4 _outFragColor;

void main() 
{
    _outFragColor = vec4(_inFragColor, 1.0);
}