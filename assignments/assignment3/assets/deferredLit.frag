#version 450 core

struct PointLight{
	vec3 position;
	float radius;
	vec4 color;
};
#define MAX_POINT_LIGHTS 64
uniform PointLight _PointLights[MAX_POINT_LIGHTS];






out vec4 FragColor; 
in vec2 UV; //From fsTriangle.vert

//All your material and lighting uniforms go here!

uniform sampler2D _MainTex; 
uniform sampler2D _ShadowMap;
uniform vec3 _EyePos;
uniform vec3 _LightDirection = vec3(0.0,-1.0,0.0);
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);
uniform float _MinBias;
uniform float _MaxBias;


uniform mat4 _ViewProjection; //Combined View->Projection Matrix
uniform mat4 _LightViewProj; //view + projection of light source camera

//layout(binding = i) can be used as an alternative to shader.setInt()
//Each sampler will always be bound to a specific texture unit
uniform layout(binding = 0) sampler2D _gPositions;
uniform layout(binding = 1) sampler2D _gNormals;
uniform layout(binding = 2) sampler2D _gAlbedo;




struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

float calcShadow(sampler2D shadowMap, vec4 lightSpacePos, vec3 normal) {
	vec3 sampleCoord = lightSpacePos.xyz / lightSpacePos.w;

	sampleCoord = sampleCoord * 0.5 + 0.5;
	float bias = max(_MaxBias * (1.0 - dot((normal), (-_LightDirection))), _MinBias);
	
	float myDepth = sampleCoord.z - bias;

	//float shadowMapDepth = texture(shadowMap, sampleCoord.xy).r;

	float totalShadow = 0;

	vec2 texelOffset = 1.0 / textureSize(shadowMap,0);

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			vec2 uv = sampleCoord.xy + vec2(x * texelOffset.x, y * texelOffset.y);

			totalShadow += step(texture(shadowMap,uv).r, myDepth);
		}
	}

	totalShadow /= 9.0;

	return totalShadow;
}

vec3 calcDirectionalLight(vec3 normal, vec3 worldPos) {
	vec3 toLight = -_LightDirection;
	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Calculate specularly reflected light
	vec3 toEye = normalize(_EyePos - worldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);

	vec4 LightSpacePos = _LightViewProj * vec4(worldPos, 1.0);

	float shadow = calcShadow(_ShadowMap, LightSpacePos, normal);
	//shadow = 0.0f;
	
	vec3 light = (_AmbientColor * _Material.Ka) + (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * (1.0 - shadow);
	return light;

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

vec3 calcPointLight(PointLight light,vec3 normal,vec3 pos){
	vec3 diff = light.position - pos;
	//Direction toward light position
	vec3 toLight = normalize(diff);
	//TODO: Usual blinn-phong calculations for diffuse + specular
	float diffuseFactor = max(dot(normal,toLight),0.0);

	vec3 toEye = normalize(_EyePos - pos);
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	vec3 lightColor = (diffuseFactor + specularFactor) * light.color.rgb;
	//Attenuation
	float d = length(diff); //Distance to light
	lightColor*=attenuateLinear(d,light.radius); //See below for attenuation options
	return lightColor;
}




void main(){
	vec3 pos = texture(_gPositions,UV).rgb;
	vec3 normal = texture(_gNormals,UV).rgb;
	vec3 totalLight = vec3(0);

	totalLight+=calcDirectionalLight(normal,pos);
	
	for(int i=0;i<MAX_POINT_LIGHTS;i++) {
		totalLight+=calcPointLight(_PointLights[i],normal,pos);
	}
	vec3 albedo = texture(_gAlbedo,UV).rgb;
	FragColor = vec4(albedo * totalLight,0);

}
