// Vulkan final shader source port.
// Source OpenGL shader: class1/interface/normaldebugG.glsl
// This file preserves the source shader's role while the final Vulkan
// renderer pipeline contracts are completed.

#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
