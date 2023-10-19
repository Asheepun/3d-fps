#version 330 core

in vec2 input_texturePosition;
in vec3 input_normal;
in float input_depth;
in float input_shadowDepth;
in float input_bigShadowDepth;
in vec2 input_shadowMapPosition;
in vec2 input_bigShadowMapPosition;

out vec4 FragColor;

uniform sampler2D colorTexture;
uniform sampler2D shadowMapDepthTexture;
uniform sampler2D grassShadowMapDepthTexture;
uniform sampler2D bigShadowMapDepthTexture;

uniform float grassShadowStrength;

uniform vec4 inputColor;

uniform float ambientLightFactor;
uniform float diffuseLightFactor;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	float shadowDepthTolerance = 0.001;
	float shadowFactor = 1.0;

	if(input_shadowMapPosition.x > 0.0 && input_shadowMapPosition.x < 1.0
	&& input_shadowMapPosition.y > 0.0 && input_shadowMapPosition.y < 1.0){

		float shadowMapDepth = texture2D(shadowMapDepthTexture, input_shadowMapPosition).r;
		float grassShadowMapDepth = texture2D(grassShadowMapDepthTexture, input_shadowMapPosition).r;

		shadowFactor = min(
			float(input_shadowDepth - shadowMapDepth < shadowDepthTolerance),
			max(float(input_shadowDepth - grassShadowMapDepth < shadowDepthTolerance), 1.0 - grassShadowStrength)
		);

	}else{

		float bigShadowMapDepth = texture2D(bigShadowMapDepthTexture, input_bigShadowMapPosition).r;

		shadowFactor = float(input_bigShadowDepth - bigShadowMapDepth < shadowDepthTolerance);
		
	}

	FragColor = texture2D(colorTexture, input_texturePosition);

	FragColor *= inputColor;

	float diffuseLight = max(dot(-lightDirection, input_normal), 0.0);

	float totalLightFactor = ambientLightFactor;

	totalLightFactor += diffuseLight * diffuseLightFactor * shadowFactor;

	FragColor.xyz *= totalLightFactor;

} 
