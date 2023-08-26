#version 330 core

precision mediump float;

in vec2 textureCoord;

uniform sampler2D tex;

uniform vec4 color;

void main(){

	gl_FragColor = color;
	gl_FragColor.w *= texture2D(tex, textureCoord).w;

	gl_FragDepth = 0.0;

}

