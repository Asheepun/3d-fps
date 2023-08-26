#version 330 core

//in vec4 input_fragmentPosition;
in vec2 input_texturePosition;
//in vec4 input_fragmentNormal;
in float input_depth;
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

uniform float grassShadowStrength;

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	if(texture2D(alphaTexture, input_texturePosition).r < 0.001){
		discard;
	}

	FragColor = texture2D(colorTexture, input_texturePosition);

	//if(FragColor.w < 0.001){
		//discard;
	//}

	//float shadowMapDepth = 1.0;
	//float grassShadowMapDepth = 1.0;
	float shadowFactor = 1.0;
	float shadowDepthTolerance = 0.0015;

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

	float totalLightFactor = ambientLightFactor + diffuseLightFactor * shadowFactor;

	FragColor.xyz *= totalLightFactor;

	//gl_FragDepth = input_depth;

} 

