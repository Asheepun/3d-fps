#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

uniform mat4 modelMatrix;
uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;

uniform sampler2D colorTexture;
uniform sampler2D shadowMapTexture;

out vec4 input_fragmentPosition;
out vec2 input_texturePosition;
out vec4 input_fragmentNormal;

void main(){

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	vertexPosition *= modelMatrix;

	input_fragmentPosition = vertexPosition;
	input_texturePosition = attribute_textureVertex;
	input_fragmentNormal = vertexNormal;

	vertexPosition *= cameraMatrix * perspectiveMatrix;

	gl_Position = vertexPosition;

	gl_Position.z = 0.0;

}
