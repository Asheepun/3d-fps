#version 330 core

in vec4 input_fragmentPosition;
in vec3 input_normal;
in vec2 input_textureCoord;

out vec4 FragColor;

uniform sampler2D voronoiTexture;
uniform sampler2D normalMap;
uniform sampler2D noiseMap;

uniform float inputT;
uniform vec3 cameraDirection;

float ambientLightFactor = 0.5;
float diffuseLightFactor = 0.5;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

int specularExponent = 20;

void main(){

	FragColor = vec4(0.0, 0.0, 1.0, 0.5);

	vec3 reflectionDirection = normalize(lightDirection - input_normal * 2.0 * dot(lightDirection, input_normal));

	float specularLight = dot(reflectionDirection, -cameraDirection);

	//FragColor.xyz = input_normal;

	//vec2 textureCoord = 0.1 + input_textureCoord * 0.8;

	/*
	float noiseFactor = texture2D(noiseMap, input_textureCoord).r;
	//noiseFactor = 0.0;

	float voronoiFactor = texture2D(voronoiTexture, input_textureCoord + vec2(0.0, noiseFactor * 0.02 - inputT * 0.02)).r;

	FragColor = vec4(0.0, 0.0, 1.0, 0.5);
	FragColor.x += voronoiFactor;
	FragColor.y += voronoiFactor;
	*/

	float diffuseLight = max(dot(-lightDirection, input_normal), 0.0);

	FragColor.xyz *= ambientLightFactor + diffuseLightFactor * diffuseLight;

	FragColor.xyz += vec3(1.0) * pow(specularLight, specularExponent);

} 

