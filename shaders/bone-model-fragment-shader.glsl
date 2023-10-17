#version 330 core

in vec4 input_fragmentPosition;
in vec4 input_fragmentNormal;
in vec4 input_boneIndices;
in vec4 input_boneWeights;

out vec4 FragColor;

//uniform mat4 perspectiveMatrix;
//uniform mat4 cameraMatrix;

uniform vec4 inputColor;

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	vec3 fragmentPosition = (input_fragmentPosition).xyz;
	vec3 fragmentNormal = (input_fragmentNormal).xyz;
	//vec3 cameraRelativeFragmentPosition = (input_fragmentPosition * cameraMatrix).xyz;

	FragColor = inputColor;

	float diffuseLight = max(dot(-lightDirection, fragmentNormal), 0.0);

	FragColor.xyz *= ambientLightFactor + diffuseLight * diffuseLightFactor;

	//FragColor.xyz = vec3(0.5, 0.5, 0.5) + fragmentNormal / 2.0;

	FragColor.w = 1.0;

	//gl_FragDepth = cameraRelativeFragmentPosition.z / 100.0;

} 
