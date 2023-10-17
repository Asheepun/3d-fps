#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec3 attribute_normalVertex;
layout (location = 2) in vec2 attribute_textureVertex;
layout (location = 3) in uvec4 attribute_boneIndices;
layout (location = 4) in vec4 attribute_boneWeights;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
//uniform mat4 perspectiveMatrix;
//uniform mat4 cameraMatrix;

uniform mat4 boneTransformations[32];

out vec4 input_fragmentPosition;
out vec4 input_fragmentNormal;
out vec4 input_boneIndices;
out vec4 input_boneWeights;

void main(){

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	mat4 boneTransformationSum = boneTransformations[int(attribute_boneIndices.x)] * attribute_boneWeights.x
							   + boneTransformations[int(attribute_boneIndices.y)] * attribute_boneWeights.y
							   + boneTransformations[int(attribute_boneIndices.z)] * attribute_boneWeights.z
							   + boneTransformations[int(attribute_boneIndices.w)] * attribute_boneWeights.w;
	input_boneIndices = attribute_boneIndices;
	input_boneWeights = attribute_boneWeights;

	vertexPosition *= boneTransformationSum;
	vertexNormal *= boneTransformationSum;
	vertexNormal = normalize(vertexNormal);

	vertexPosition *= modelMatrix;

	input_fragmentPosition = vertexPosition;
	input_fragmentNormal = vertexNormal;

	//vertexPosition *= cameraMatrix * perspectiveMatrix;

	gl_Position = vertexPosition * viewMatrix;

	//gl_Position.z = 0.0;

}
