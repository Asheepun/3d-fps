#version 330 core

in vec2 input_texturePosition;
in float input_depth;
in float input_shadowDepth;
in vec2 input_shadowMapPosition;

out vec4 FragColor;

uniform sampler2D alphaTexture;

uniform float shadowStrength;

void main(){

	if(texture2D(alphaTexture, input_texturePosition).r < 0.001){
		discard;
	}

	//float depth = input_depth + float(alpha < 0.001);
	//float depth = input_depth;
	
	FragColor = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0);
	//FragColor = vec4(depth, depth, depth, depth);
	//FragColor = vec4(0.0, 0.0, 0.0, 0.0);

	//gl_FragDepth = 0.0;
	//gl_FragDepth = gl_FragCoord.z + 0.001;

}
