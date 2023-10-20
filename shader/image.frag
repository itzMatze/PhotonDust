#version 460

layout(location = 0) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D image;

void main()
{
    out_color = texture(image, frag_tex);
}
