#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float exposure;
uniform float bloomStrength = 0.04f;

vec3 bloom()
{
	vec3 hdrColor = texture(scene, TexCoords).rgb;
	vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
	return mix(hdrColor, bloomColor, bloomStrength);
}

void main()
{
	vec3 result = vec3(0.0);
	result = bloom();
	result = vec3(1.0) - exp(-result * exposure);
	//Gamma correction
	const float gamma = 2.2;
	result = pow(result, vec3(1.0/gamma));
	FragColor = vec4(result, 1.0);
}