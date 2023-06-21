#version 330 core

in vec4 input_fragmentPosition;
in vec4 input_fragmentNormal;

uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;

out vec4 FragColor;

float ambientLightFactor = 0.1;
float diffuseLightFactor = 0.9;

vec3 lightDirection = vec3(1.0, -1.0, 1.0);

void main(){

	vec3 fragmentPosition = (input_fragmentPosition).xyz;
	vec3 fragmentNormal = (input_fragmentNormal).xyz;
	vec3 cameraRelativeFragmentPosition = (input_fragmentPosition * cameraMatrix).xyz;

	FragColor = vec4(0.14, 0.55, 0.17, 1.0);

	float diffuseLight = max(dot(-lightDirection, fragmentNormal), 0.0);

	FragColor.xyz *= ambientLightFactor + diffuseLight * diffuseLightFactor;

	//FragColor = input_fragmentNormal;
	//FragColor.w = 1.0;

	gl_FragDepth = cameraRelativeFragmentPosition.z / 100.0;

} 
