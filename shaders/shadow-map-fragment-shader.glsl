#version 330 core
in float depth;

uniform mat4 modelMatrix;
uniform mat4 modelRotationMatrix;
uniform mat4 cameraMatrix;
uniform vec4 inputColor;

void main(){

	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);

	gl_FragDepth = depth;

} 
