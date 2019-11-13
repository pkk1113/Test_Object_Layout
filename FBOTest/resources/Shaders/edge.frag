#version 430 core

layout(binding = 0) uniform sampler2D map;

in vec2 texCoord;

layout(location = 0) out vec4 frag_color;

// »þÇÁ´×
const float kernel[9] = {
	-1.f, -1.f, -1.f,
	-1.f, 8.f, -1.f,
	-1.f, -1.f, -1.f,
};

const float s = 0.01f;
const vec2 steps[9] = {
	vec2(-s, s), vec2(0, -s), vec2(s, -s),
	vec2(-s, 0), vec2(0, 0), vec2(s, 0),
	vec2(-s, s), vec2(0, s), vec2(s, s)
};

void main()
{
	vec4 value = vec4(0.f);

	for(int i = 0; i < 9; i++) {
		value += (kernel[i] * texture(map, texCoord + steps[i]));
	}

	if(value.a > 0.f)
		value = vec4(1, 0, 0, 1);
	else
		value = vec4(0, 0, 0, 0);

	frag_color = value;
}