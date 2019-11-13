#version 430 core

layout(location = 3) uniform float alpha;

layout(location = 0) out vec4 frag_surface;
layout(location = 1) out vec4 frag_world_normal;

in vec3 world_normal;

void main()
{
	frag_surface = vec4(0, 0, 0, alpha);
	frag_world_normal = vec4(world_normal, 1);
}