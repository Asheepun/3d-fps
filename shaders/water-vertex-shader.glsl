#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

out vec4 input_fragmentPosition;

uniform mat4 rotationMatrix;
uniform mat4 scalingMatrix;
uniform mat4 translationMatrix;
uniform mat4 viewMatrix;

void main(){

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	vertexPosition *= rotationMatrix;

	input_fragmentPosition = vertexPosition;

	vertexPosition *= scalingMatrix;

	vertexPosition *= translationMatrix;

	gl_Position = vertexPosition * viewMatrix;

}

