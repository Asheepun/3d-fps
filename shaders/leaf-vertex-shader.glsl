#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

out vec2 input_texturePosition;
out float input_shadowDepth;
out float input_bigShadowDepth;
out vec2 input_shadowMapPosition;
out vec2 input_bigShadowMapPosition;

uniform sampler2D colorTexture;
uniform samplerBuffer modelTransformationsBuffer;
uniform sampler2D shadowMapTexture;

uniform mat4 viewMatrix;
uniform mat4 lightViewMatrix;
uniform mat4 bigLightViewMatrix;

uniform float windTime;

void main(){

	input_texturePosition = attribute_textureVertex;

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);

	mat4 modelTransformations = mat4(
		texelFetch(modelTransformationsBuffer, gl_InstanceID * 4 + 0),
		texelFetch(modelTransformationsBuffer, gl_InstanceID * 4 + 1),
		texelFetch(modelTransformationsBuffer, gl_InstanceID * 4 + 2),
		texelFetch(modelTransformationsBuffer, gl_InstanceID * 4 + 3)
	);

	vertexPosition.y += 1.0;

	vertexPosition.z += 0.1 * sin(windTime + modelTransformations[0][3] + modelTransformations[2][3]) * vertexPosition.y + 0.05 * sin(windTime * 5.0);

	vertexPosition *= modelTransformations;

	vec4 lightProjectedVertexPosition = vertexPosition * lightViewMatrix;
	input_shadowDepth = 0.5 * lightProjectedVertexPosition.z + 0.5;
	input_shadowMapPosition = (vec2(1.0, 1.0) + lightProjectedVertexPosition.xy) / 2.0;

	vec4 bigLightProjectedVertexPosition = vertexPosition * bigLightViewMatrix;
	input_bigShadowDepth = 0.5 * bigLightProjectedVertexPosition.z + 0.5;
	input_bigShadowMapPosition = (vec2(1.0, 1.0) + bigLightProjectedVertexPosition.xy) / 2.0;

	gl_Position = vertexPosition * viewMatrix;

}

