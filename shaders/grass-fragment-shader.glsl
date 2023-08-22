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

	float shadowMapDepth = texture2D(shadowMapDepthTexture, input_shadowMapPosition).r;
	vec4 shadowMapData = texture2D(shadowMapDataTexture, input_shadowMapPosition);
	float transparentShadowDepth = shadowMapData.a;
	float transparentShadowStrength = shadowMapData.r;

	//float shadowDepthDiff = input_shadowDepth - shadowMapDepth;

	float shadowDepthTolerance = 0.0015;
	float shadowFactor = min(
		float(input_shadowDepth - shadowMapDepth < shadowDepthTolerance),
		max(float(input_shadowDepth - transparentShadowDepth < shadowDepthTolerance), 1.0 - transparentShadowStrength)
	);
	//bool inShadow = shadowDepthDiff > shadowDepthTolerance;

	float diffuseLight = 1.0;

	//float totalLightFactor = float(!inShadow) * diffuseLight * diffuseLightFactor + ambientLightFactor;
	//
	float totalLightFactor = ambientLightFactor + diffuseLight * diffuseLightFactor * shadowFactor;

	FragColor.xyz *= totalLightFactor;

	//gl_FragDepth = input_depth + float(FragColor.w < 0.001);
	gl_FragDepth = input_depth;

} 

