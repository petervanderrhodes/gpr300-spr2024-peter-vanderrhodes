#version 450 core

struct PointLight{
	vec3 position;
	float radius;
	vec4 color;
};
#define MAX_POINT_LIGHTS 64
uniform PointLight _PointLights[MAX_POINT_LIGHTS];

//vec3 calcDirectionalLight(vec3 pos) {

//}




out vec4 FragColor; 
in vec2 UV; //From fsTriangle.vert

//All your material and lighting uniforms go here!

uniform vec3 _EyePos;

struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

vec3 calcPointLight(PointLight light,vec3 normal,vec3 pos){
	vec3 diff = light.position - pos;
	//Direction toward light position
	vec3 toLight = normalize(diff);
	//TODO: Usual blinn-phong calculations for diffuse + specular
	float diffuseFactor = max(dot(normal,toLight),0.0);

	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	vec3 lightColor = (diffuseFactor + specularFactor) * light.color;
	//Attenuation
	float d = length(diff); //Distance to light
	lightColor*=attenuateLinear(d,light.radius); //See below for attenuation options
	return lightColor;
}

//Linear falloff
float attenuateLinear(float distance, float radius){
	return clamp((radius-distance)/radius,0.0,1.0);
}

//Exponential falloff
float attenuateExponential(float distance, float radius){
	float i = clamp(1.0 - pow(distance/radius,4.0),0.0,1.0);
	return i * i;
}

//layout(binding = i) can be used as an alternative to shader.setInt()
//Each sampler will always be bound to a specific texture unit
uniform layout(binding = 0) sampler2D _gPositions;
uniform layout(binding = 1) sampler2D _gNormals;
uniform layout(binding = 2) sampler2D _gAlbedo;

void main(){
	vec3 pos = texture(_gPositions,UV).rgb;
	vec3 normal = texture(_gNormals,UV).rgb;
	vec3 totalLight = vec3(0);
	//totalLight+=calcDirectionalLight(_MainLight,normal,pos);
	for(int i=0;i<MAX_POINT_LIGHTS;i++) {
		totalLight+=calcPointLight(_PointLights[i],normal,pos);
	}
	vec3 albedo = texture(_gAlbedo,UV).rgb;
	FragColor = vec4(albedo * totalLight,0);

}
