#version 430 core

layout(binding = 0) uniform sampler2D color;
layout(binding = 1) uniform sampler2D edge;

in vec2 texCoord;

layout(location = 0) out vec4 frag_color;

void main()
{
	vec4 c = texture(color, texCoord);
	vec4 e = texture(edge, texCoord);

	frag_color = c * (1.f - e.a) + e;
}