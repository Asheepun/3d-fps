#version 330 core

in vec4 input_fragmentPosition;
in vec4 input_fragmentNormal;

uniform mat4 cameraMatrix;

void main(){

	vec3 cameraRelativeFragmentPosition = (input_fragmentPosition * cameraMatrix).xyz;

	float fragmentDepth = cameraRelativeFragmentPosition.z / 100.0;

	gl_FragColor = vec4(fragmentDepth, fragmentDepth, fragmentDepth, 1.0);

	gl_FragDepth = fragmentDepth;

} 
