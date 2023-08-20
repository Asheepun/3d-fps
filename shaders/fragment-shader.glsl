#version 330 core

in vec4 input_fragmentPosition;
in vec4 input_fragmentNormal;

out vec4 FragColor;

uniform sampler2D shadowMapTexture;

uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;
uniform mat4 lightCameraMatrix;
uniform mat4 lightPerspectiveMatrix;

uniform vec4 inputColor;

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	vec3 fragmentPosition = (input_fragmentPosition).xyz;
	vec3 fragmentNormal = (input_fragmentNormal).xyz;
	vec3 cameraRelativeFragmentPosition = (input_fragmentPosition * cameraMatrix).xyz;
	vec4 lightRelativeFragmentPosition = input_fragmentPosition * lightCameraMatrix;
	vec2 shadowMapFragmentPosition = (lightRelativeFragmentPosition * lightPerspectiveMatrix).xy;

	float fragmentShadowDepth = lightRelativeFragmentPosition.z / 100.0;
	float shadowMapDepth = texture2D(shadowMapTexture, (vec2(1.0, 1.0) + shadowMapFragmentPosition) / 2.0).r;

	float shadowDepthTolerance = 0.001;
	bool inShadow = fragmentShadowDepth - shadowMapDepth > shadowDepthTolerance;

	FragColor = inputColor;

	float diffuseLight = max(dot(-lightDirection, fragmentNormal), 0.0);

	float totalLightFactor = ambientLightFactor;

	if(!inShadow){
		totalLightFactor += diffuseLight * diffuseLightFactor;
	}

	FragColor.xyz *= totalLightFactor;

	gl_FragDepth = cameraRelativeFragmentPosition.z / 100.0;;

} 
