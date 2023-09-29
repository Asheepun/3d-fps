#version 330 core

in vec3 input_staticWorldPosition;
in vec2 input_texturePosition;
in float input_shadowDepth;
in float input_bigShadowDepth;
in vec2 input_shadowMapPosition;
in vec2 input_bigShadowMapPosition;

out vec4 FragColor;

uniform sampler2D colorTexture;
uniform sampler2D alphaTexture;
uniform samplerBuffer grassPositions;
uniform sampler2D shadowMapDepthTexture;
uniform sampler2D grassShadowMapDepthTexture;
uniform sampler2D bigShadowMapDepthTexture;
uniform sampler2D paintMapTexture;

uniform float grassShadowStrength;

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

//float paintHeight = 2.0;
float paintHeightFactor = 10.0;
float paintWidth = 0.2;

void main(){

	FragColor = texture2D(colorTexture, input_texturePosition);

	if(FragColor.w < 0.001){
		discard;
	}

	float paintHeight = texture2D(paintMapTexture, input_staticWorldPosition.xz / 100.0).r;

	float isBloody = float(paintHeight > 0.0 && abs(paintHeight - (1.0 - input_texturePosition.y)) < paintWidth);
	FragColor = isBloody * vec4(0.9, 0.0, 0.0, 1.0) + (1.0 - isBloody) * FragColor;

	float shadowFactor = 1.0;
	float shadowDepthTolerance = 0.0015;

	float shadowMapDepth = texture2D(shadowMapDepthTexture, input_shadowMapPosition).r;
	float grassShadowMapDepth = texture2D(grassShadowMapDepthTexture, input_shadowMapPosition).r;
	float bigShadowMapDepth = texture2D(bigShadowMapDepthTexture, input_bigShadowMapPosition).r;

	shadowFactor = min(
		float(input_shadowDepth - shadowMapDepth < shadowDepthTolerance) * float(input_bigShadowDepth - bigShadowMapDepth < shadowDepthTolerance),
		1.0 - float(input_shadowDepth - grassShadowMapDepth > shadowDepthTolerance) * grassShadowStrength
	);

	float totalLightFactor = diffuseLightFactor * shadowFactor + ambientLightFactor;

	FragColor.xyz *= totalLightFactor;

} 

