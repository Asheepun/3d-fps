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

uniform mat4 viewMatrix;
uniform mat4 lightViewMatrix;
uniform mat4 bigLightViewMatrix;

uniform sampler2D colorTexture;
uniform sampler2D shadowMapTexture;

void main(){

	input_texturePosition = attribute_textureVertex;

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	input_normal = vertexNormal.xyz;

	vertexPosition *= modelMatrix;

	vec4 lightProjectedVertexPosition = vertexPosition * lightViewMatrix;
	input_shadowDepth = 0.5 * lightProjectedVertexPosition.z + 0.5;
	input_shadowMapPosition = (vec2(1.0, 1.0) + lightProjectedVertexPosition.xy) / 2.0;

	vec4 bigLightProjectedVertexPosition = vertexPosition * bigLightViewMatrix;
	input_bigShadowDepth = 0.5 * bigLightProjectedVertexPosition.z + 0.5;
	input_bigShadowMapPosition = (vec2(1.0, 1.0) + bigLightProjectedVertexPosition.xy) / 2.0;

	gl_Position = vertexPosition * viewMatrix;

}
