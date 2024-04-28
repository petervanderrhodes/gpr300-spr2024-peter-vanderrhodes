#version 330 core
out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D _ColorBuffer;
uniform sampler2D bloomBlur;
uniform float exposure;
uniform float bloomStrength = 0.04f;

vec3 bloom()
{
	vec3 hdrColor = texture(_ColorBuffer, texCoord).rgb;
	vec3 bloomColor = texture(bloomBlur, texCoord).rgb;
	return mix(hdrColor, bloomColor, bloomStrength);
}

void main()
{
	vec3 result = vec3(0.0);
	result = texture(_ColorBuffer, texCoord).rgb;
	//result = vec3(1.0) - exp(-result * exposure);
	//Gamma correction
	//const float gamma = 2.2;
	//result = pow(result, vec3(1.0/gamma));
	FragColor = vec4(result, 1.0);
}