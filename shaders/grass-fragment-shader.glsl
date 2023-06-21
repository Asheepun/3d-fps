#version 330 core

in vec4 input_fragmentPosition;
in vec2 input_texturePosition;
in vec4 input_fragmentNormal;

out vec4 FragColor;

uniform sampler2D colorTexture;
uniform sampler2D heightMap;

uniform mat4 modelMatrix;
uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;

float ambientLightFactor = 0.3;
float diffuseLightFactor = 0.7;

vec3 lightDirection = vec3(0.5, -1.0, 0.5);

void main(){

	vec3 fragmentPosition = (input_fragmentPosition).xyz;
	vec3 fragmentNormal = (input_fragmentNormal).xyz;
	vec3 cameraRelativeFragmentPosition = (input_fragmentPosition * cameraMatrix).xyz;

	//FragColor = vec4(input_texturePosition.x, input_texturePosition.y, 0.0, 1.0);
	FragColor = texture2D(colorTexture, input_texturePosition.xy);

	//FragColor = vec4(1.0, 0.0, 0.0, 1.0);

	gl_FragDepth = cameraRelativeFragmentPosition.z / 100.0 + float(FragColor.w < 0.001);

} 

