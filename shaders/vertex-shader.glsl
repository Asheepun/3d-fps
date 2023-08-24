#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

out vec2 input_texturePosition;
out vec3 input_normal;
out float input_depth;
out float input_shadowDepth;
out float input_bigShadowDepth;
out vec2 input_shadowMapPosition;
out vec2 input_bigShadowMapPosition;

uniform mat4 modelMatrix;

uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;
uniform mat4 lightCameraMatrix;
uniform mat4 lightPerspectiveMatrix;
uniform mat4 bigLightCameraMatrix;
uniform mat4 bigLightPerspectiveMatrix;

uniform sampler2D colorTexture;
uniform sampler2D shadowMapTexture;

void main(){

	input_texturePosition = attribute_textureVertex;

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	input_normal = vertexNormal.xyz;

	vertexPosition *= modelMatrix;

	vec4 lightRelativeVertexPosition = vertexPosition * lightCameraMatrix;
	vec2 shadowMapVertexPosition = (lightRelativeVertexPosition * lightPerspectiveMatrix).xy;

	input_shadowDepth = lightRelativeVertexPosition.z / 100.0;
	input_shadowMapPosition = (vec2(1.0, 1.0) + shadowMapVertexPosition) / 2.0;

	vec4 bigLightRelativeVertexPosition = vertexPosition * bigLightCameraMatrix;
	vec2 bigShadowMapVertexPosition = (bigLightRelativeVertexPosition * bigLightPerspectiveMatrix).xy;

	input_bigShadowDepth = bigLightRelativeVertexPosition.z / 100.0;
	input_bigShadowMapPosition = (vec2(1.0, 1.0) + bigShadowMapVertexPosition) / 2.0;

	//input_fragmentPosition = vertexPosition;

	vertexPosition *= cameraMatrix;

	input_depth = vertexPosition.z / 100.0;

	vertexPosition *= perspectiveMatrix;

	gl_Position = vertexPosition;

	gl_Position.z = 0.0;

}
