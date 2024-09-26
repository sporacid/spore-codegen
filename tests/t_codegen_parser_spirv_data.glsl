#version 450

#pragma shader_stage(vertex)

layout (location = 0) in vec3 input0;
layout (location = 1) in vec3 input1;
layout (location = 2) in vec2 input2[4];

layout (location = 0) out vec3 output0;
layout (location = 1) out vec3 output1;
layout (location = 2) out vec2 output2[4];

layout (push_constant) uniform constants
{
    vec2 constant0;
    float constant1;
};

layout (set = 0, binding = 0) uniform descriptors
{
    vec2 descriptor0;
    float descriptor1;
};

layout (set = 1, binding = 0) uniform sampler2D sampler0;
layout (set = 1, binding = 1) uniform sampler2D sampler1;

void main()
{
    gl_Position = vec4(0);
}