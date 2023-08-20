#version 330 core

in vec4 input_fragmentPosition;
in vec2 input_texturePosition;
in vec4 input_fragmentNormal;
in float input_depth;
in float input_shadowDepth;
in vec2 input_shadowMapPosition;

out vec4 FragColor;

uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;
uniform mat4 lightCameraMatrix;
uniform mat4 lightPerspectiveMatrix;

uniform sampler2D colorTexture;
uniform samplerBuffer modelTransformationsBuffer;
uniform sampler2D shadowMapTexture;

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	vec3 fragmentPosition = (input_fragmentPosition).xyz;
	vec3 fragmentNormal = (input_fragmentNormal).xyz;
	//vec3 cameraRelativeFragmentPosition = (input_fragmentPosition * cameraMatrix).xyz;
	//vec4 lightRelativeFragmentPosition = input_fragmentPosition * lightCameraMatrix;
	//vec2 shadowMapFragmentPosition = (lightRelativeFragmentPosition * lightPerspectiveMatrix).xy;

	//float fragmentShadowDepth = lightRelativeFragmentPosition.z / 100.0;
	float shadowMapDepth = texture2D(shadowMapTexture, input_shadowMapPosition).r;
	float shadowDepthDiff = input_shadowDepth - shadowMapDepth;

	//float leafTolerance = 0.01;
	float shadowDepthTolerance = 0.0005;
	bool inShadow = shadowDepthDiff > shadowDepthTolerance;

	//FragColor = inputColor;
	FragColor = texture2D(colorTexture, input_texturePosition);

	//float diffuseLight = max(dot(-lightDirection, fragmentNormal), 0.0);

	//float diffuseLight = max(dot(-lightDirection, fragmentNormal), 0.0);
	float diffuseLight = 1.5;

	float totalLightFactor = ambientLightFactor;

	if(!inShadow){
		totalLightFactor += diffuseLight * diffuseLightFactor;
	}

	//make shadows not sharp between leaves
	if(inShadow){
		totalLightFactor += diffuseLightFactor * max(0.0, 0.5 - 10.0 * (shadowDepthDiff));
	}

	FragColor.xyz *= totalLightFactor;

	gl_FragDepth = input_depth + float(FragColor.w < 0.001);

} 

