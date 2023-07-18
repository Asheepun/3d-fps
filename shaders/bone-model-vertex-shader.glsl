#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 2) in vec3 attribute_normalVertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 3) in uvec4 attribute_boneIndices;
layout (location = 4) in vec4 attribute_boneWeights;

uniform mat4 modelMatrix;
uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;

uniform mat4 boneTransformations[32];
uniform mat4 inverseBoneTransformations[32];

out vec4 input_fragmentPosition;
out vec4 input_fragmentNormal;

void main(){

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	vertexNormal.xyz = vertexNormal.yxz;

	//vertexNormal.x *= -1.0;
	//vertexNormal.yz = vertexNormal.zy;

	vertexPosition *= boneTransformations[attribute_boneIndices.x] * attribute_boneWeights.x;

	vertexPosition *= modelMatrix;

	input_fragmentPosition = vertexPosition;
	input_fragmentNormal = vertexNormal;

	vertexPosition *= cameraMatrix * perspectiveMatrix;

	gl_Position = vertexPosition;

	gl_Position.z = 0.0;

}
