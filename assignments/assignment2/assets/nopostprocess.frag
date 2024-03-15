#version 450
//Default shader

out vec4 FragColor;

in vec2 UV;

uniform sampler2D _ColorBuffer;

void main() {
	vec3 color = texture(_ColorBuffer, UV).rgb;

	FragColor = vec4(color, 1.0);
}
