#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

out vec4 input_fragmentPosition;
out vec2 input_texturePosition;
out vec4 input_fragmentNormal;
out float input_depth;
out float input_shadowDepth;
out vec2 input_shadowMapPosition;

uniform sampler2D colorTexture;
uniform samplerBuffer modelTransformationsBuffer;
uniform sampler2D shadowMapTexture;

//uniform mat4 modelMatrix;
uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;
uniform mat4 lightCameraMatrix;
uniform mat4 lightPerspectiveMatrix;

uniform float windTime;

void main(){

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec2 texturePosition = attribute_textureVertex;
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	mat4 modelTransformations = mat4(
		texelFetch(modelTransformationsBuffer, gl_InstanceID * 4 + 0),
		texelFetch(modelTransformationsBuffer, gl_InstanceID * 4 + 1),
		texelFetch(modelTransformationsBuffer, gl_InstanceID * 4 + 2),
		texelFetch(modelTransformationsBuffer, gl_InstanceID * 4 + 3)
	);

	vertexPosition.y += 1.0;

	vertexPosition.z += 0.1 * sin(windTime + modelTransformations[0][3] + modelTransformations[2][3]) * vertexPosition.y + 0.05 * sin(windTime * 5.0);

	//vertexPosition *= modelMatrix;
	vertexPosition *= modelTransformations;

	input_fragmentPosition = vertexPosition;
	input_texturePosition = texturePosition;
	input_fragmentNormal = vertexNormal;

	vec4 lightRelativeVertexPosition = vertexPosition * lightCameraMatrix;
	vec2 shadowMapVertexPosition = (lightRelativeVertexPosition * lightPerspectiveMatrix).xy;

	input_shadowDepth = lightRelativeVertexPosition.z / 100.0;
	input_shadowMapPosition = (vec2(1.0, 1.0) + shadowMapVertexPosition) / 2.0;

	vertexPosition *= cameraMatrix;

	input_depth = vertexPosition.z / 100.0;

	vertexPosition *= perspectiveMatrix;

	gl_Position = vertexPosition;

	gl_Position.z = 0.0;

}

