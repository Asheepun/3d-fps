#version 330 core

//in vec4 input_fragmentPosition;
in vec2 input_texturePosition;
//in vec4 input_fragmentNormal;
in float input_depth;
in float input_shadowDepth;
in vec2 input_shadowMapPosition;

out vec4 FragColor;

uniform sampler2D colorTexture;
uniform samplerBuffer grassPositions;
uniform sampler2D shadowMapTexture;

/*
uniform mat4 modelMatrix;
uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;
uniform mat4 lightCameraMatrix;
uniform mat4 lightPerspectiveMatrix;
*/

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	//vec3 fragmentPosition = (input_fragmentPosition).xyz;
	//vec3 fragmentNormal = (input_fragmentNormal).xyz;
	//vec3 cameraRelativeFragmentPosition = (input_fragmentPosition * cameraMatrix).xyz;
	//vec4 lightRelativeFragmentPosition = input_fragmentPosition * lightCameraMatrix;
	//vec2 shadowMapFragmentPosition = (lightRelativeFragmentPosition * lightPerspectiveMatrix).xy;

	//float fragmentShadowDepth = lightRelativeFragmentPosition.z / 100.0;
	//float shadowMapDepth = texture2D(shadowMapTexture, (vec2(1.0, 1.0) + shadowMapFragmentPosition) / 2.0).r;
	float shadowMapDepth = texture2D(shadowMapTexture, input_shadowMapPosition).r;
	float shadowDepthDiff = input_shadowDepth - shadowMapDepth;

	float shadowDepthTolerance = 0.0015;
	bool inShadow = shadowDepthDiff > shadowDepthTolerance;

	//FragColor = vec4(input_texturePosition.x, input_texturePosition.y, 0.0, 1.0);
	FragColor = texture2D(colorTexture, input_texturePosition.xy);

	float diffuseLight = 1.0;

	float totalLightFactor = ambientLightFactor;

	if(!inShadow){
		totalLightFactor += diffuseLight * diffuseLightFactor;
	}

	FragColor.xyz *= totalLightFactor;

	//FragColor = vec4(1.0, 0.0, 0.0, 1.0);

	//gl_FragDepth = cameraRelativeFragmentPosition.z / 100.0 + float(FragColor.w < 0.001);
	gl_FragDepth = input_depth + float(FragColor.w < 0.001);

} 

