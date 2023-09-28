#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

out vec3 input_staticWorldPosition;
out vec2 input_texturePosition;
out float input_shadowDepth;
out float input_bigShadowDepth;
out vec2 input_shadowMapPosition;
out vec2 input_bigShadowMapPosition;

uniform sampler2D colorTexture;
uniform samplerBuffer grassPositions;
uniform sampler2D shadowMapTexture;
uniform sampler2D paintMapTexture;

uniform mat4 viewMatrix;
uniform mat4 lightViewMatrix;
uniform mat4 bigLightViewMatrix;

uniform mat2 rotationMatrix;

uniform float windTime;

uniform int n_grassPositions;

uniform vec2 cameraPos;

void main(){

	input_texturePosition = attribute_textureVertex;

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);

	vec4 position = texelFetch(grassPositions, gl_InstanceID);

	float cutHeight = floor(position.w) / 100.0;
	float seed = position.w - cutHeight * 100.0;

	vertexPosition.y = (vertexPosition.y + 1.0) * (cutHeight) - 1.0;
	input_texturePosition.y = (input_texturePosition.y - 1.0) * (cutHeight) + 1.0;

	vertexPosition.xz *= rotationMatrix;

	vertexPosition.xyz += position.xyz;

	input_staticWorldPosition = vertexPosition.xyz;

	float timeOffset = sin(vertexPosition.x / 100.0) + seed * 3.14;
	float heightFactor = (1.0 + attribute_vertex.y) / 2.0;
	vertexPosition.x += 0.2 * (sin(windTime + timeOffset)) * heightFactor * heightFactor * heightFactor;

	vec4 lightProjectedVertexPosition = vertexPosition * lightViewMatrix;
	input_shadowDepth = 0.5 * lightProjectedVertexPosition.z + 0.5;
	input_shadowMapPosition = (vec2(1.0, 1.0) + lightProjectedVertexPosition.xy) / 2.0;

	vec4 bigLightProjectedVertexPosition = vertexPosition * bigLightViewMatrix;
	input_bigShadowDepth = 0.5 * bigLightProjectedVertexPosition.z + 0.5;
	input_bigShadowMapPosition = (vec2(1.0, 1.0) + bigLightProjectedVertexPosition.xy) / 2.0;

	gl_Position = vertexPosition * viewMatrix;

}

