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

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

vec3 lightDirection = vec3(0.7, -1.0, 0.5);

void main(){

	FragColor = texture2D(colorTexture, input_texturePosition.xy);

	float shadowMapDepth = texture2D(shadowMapTexture, input_shadowMapPosition).r;
	float shadowDepthDiff = input_shadowDepth - shadowMapDepth;

	float shadowDepthTolerance = 0.0015;
	bool inShadow = shadowDepthDiff > shadowDepthTolerance;

	float diffuseLight = 1.0;

	float totalLightFactor = float(!inShadow) * diffuseLight * diffuseLightFactor + ambientLightFactor;

	FragColor.xyz *= totalLightFactor;

	gl_FragDepth = input_depth + float(FragColor.w < 0.001);

} 

