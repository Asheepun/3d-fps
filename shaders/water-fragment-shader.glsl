#version 330 core

in vec4 input_fragmentPosition;

out vec4 FragColor;

uniform float inputT;

float ambientLightFactor = 0.5;
float diffuseLightFactor = 0.3;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	FragColor = vec4(0.5, 0.7, 0.9, 1.0);
	FragColor.xy *= 0.5;

	float t = input_fragmentPosition.x * 100.0 - inputT * 10.0;

	vec3 normal = normalize(vec3(1.0, cos(t) + cos(sin(t) / 10.0), 0.0));

	//normal = vec3(0.0, 1.0, 0.0);

	float diffuseLight = abs(dot(-lightDirection, normal));

	float totalLightFactor = ambientLightFactor + diffuseLight * diffuseLightFactor;

	FragColor.xyz *= totalLightFactor;

	//normal.y = 0.5 + normal.y * 0.5;
	//FragColor.xyz = normal;
	//FragColor = vec4(0.0, 0.0, 0.5 * sin(input_fragmentPosition.x * 10.0) + 0.5, 1.0);

} 

