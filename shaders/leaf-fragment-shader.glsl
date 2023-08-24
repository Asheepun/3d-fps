#version 330 core

in vec4 input_fragmentPosition;
in vec2 input_texturePosition;
in vec4 input_fragmentNormal;
in float input_depth;
in float input_shadowDepth;
in float input_bigShadowDepth;
in vec2 input_shadowMapPosition;
in vec2 input_bigShadowMapPosition;

out vec4 FragColor;

uniform sampler2D colorTexture;
uniform samplerBuffer modelTransformationsBuffer;
uniform sampler2D shadowMapDepthTexture;
uniform sampler2D shadowMapDataTexture;
uniform sampler2D bigShadowMapDepthTexture;

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

	float shadowDepthTolerance = 0.0005;

	float shadowFactor = 1.0;
	
	if(input_shadowMapPosition.x > 0.0 && input_shadowMapPosition.x < 1.0
	&& input_shadowMapPosition.y > 0.0 && input_shadowMapPosition.y < 1.0){


		float shadowMapDepth = texture2D(shadowMapDepthTexture, input_shadowMapPosition).r;
		float shadowDepthDiff = input_shadowDepth - shadowMapDepth;

		shadowFactor = float(shadowDepthDiff < shadowDepthTolerance) + max(0.0, 0.5 - 10.0 * (shadowDepthDiff));

	}else{

		float bigShadowMapDepth = texture2D(bigShadowMapDepthTexture, input_bigShadowMapPosition).r;
		float bigShadowDepthDiff = input_bigShadowDepth - bigShadowMapDepth;

		shadowFactor = float(bigShadowDepthDiff < shadowDepthTolerance) + max(0.0, 0.5 - 10.0 * (bigShadowDepthDiff));
	
	}

	shadowFactor = min(shadowFactor, 1.0);

	float diffuseLight = 1.5;

	float totalLightFactor = ambientLightFactor;

	totalLightFactor += diffuseLight * diffuseLightFactor * shadowFactor;

	FragColor.xyz *= totalLightFactor;

	gl_FragDepth = input_depth;

} 

