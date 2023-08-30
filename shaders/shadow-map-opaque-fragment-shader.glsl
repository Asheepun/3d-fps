#version 330 core

in vec2 input_texturePosition;
in float input_depth;
in float input_shadowDepth;
in vec2 input_shadowMapPosition;

out vec4 FragColor;

uniform sampler2D alphaTexture;

void main(){

	FragColor = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0);

}
