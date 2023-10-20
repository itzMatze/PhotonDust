#version 460

layout (location = 0) out vec2 frag_tex;

void main() 
{
	frag_tex = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(frag_tex.x * 2.0f - 1.0f, -2.0f * frag_tex.y + 1.0f, 0.0f, 1.0f);
}
