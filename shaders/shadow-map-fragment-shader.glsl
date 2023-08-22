#version 330 core

in vec2 input_texturePosition;
in float input_depth;
in float input_shadowDepth;
in vec2 input_shadowMapPosition;

out vec4 FragColor;

uniform sampler2D colorTexture;

uniform float shadowStrength;

void main(){

	float alpha = texture2D(colorTexture, input_texturePosition).a;

	if(alpha < 0.001){
		discard;
	}

	//float depth = input_depth + float(alpha < 0.001);
	float depth = input_depth;
	
	FragColor = vec4(shadowStrength, shadowStrength, shadowStrength, depth);
	//FragColor = vec4(0.0, 0.0, 0.0, 0.0);

	//gl_FragDepth = 0.0;
	gl_FragDepth = depth;

}
