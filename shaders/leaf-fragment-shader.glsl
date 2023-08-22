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
uniform sampler2D shadowMapDepthTexture;
uniform sampler2D shadowMapDataTexture;

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	FragColor = texture2D(colorTexture, input_texturePosition);

	if(FragColor.w < 0.001){
		discard;
	}

	vec3 fragmentPosition = (input_fragmentPosition).xyz;
	vec3 fragmentNormal = (input_fragmentNormal).xyz;

	float shadowMapDepth = texture2D(shadowMapDepthTexture, input_shadowMapPosition).r;
	//vec4 shadowMapData = texture2D(shadowMapDataTexture, input_shadowMapPosition);
	//float transparentShadowDepth = shadowMapData.a;
	//float transparentShadowStrength = shadowMapData.r;

	float shadowDepthTolerance = 0.0005;
	float shadowDepthDiff = input_shadowDepth - shadowMapDepth;
	//float shadowFactor = min(
		//float(input_shadowDepth - shadowMapDepth < shadowDepthTolerance),
		//max(float(input_shadowDepth - transparentShadowDepth < shadowDepthTolerance), 1.0 - transparentShadowStrength)
	//);
	float shadowFactor = min(
		float(shadowDepthDiff < shadowDepthTolerance) + max(0.0, 0.5 - 10.0 * (shadowDepthDiff)),
		1.0
	);

	//FragColor = inputColor;

	//float diffuseLight = max(dot(-lightDirection, fragmentNormal), 0.0);

	//float diffuseLight = max(dot(-lightDirection, fragmentNormal), 0.0);
	float diffuseLight = 1.5;

	float totalLightFactor = ambientLightFactor;

	totalLightFactor += diffuseLight * diffuseLightFactor * shadowFactor;

	/*
	if(!inShadow){
		totalLightFactor += diffuseLight * diffuseLightFactor;
	}

	//make shadows not sharp between leaves
	if(inShadow){
		totalLightFactor += diffuseLightFactor * max(0.0, 0.5 - 10.0 * (shadowDepthDiff));
	}
	*/

	FragColor.xyz *= totalLightFactor;

	gl_FragDepth = input_depth;
	//gl_FragDepth = input_depth + fl;

} 

