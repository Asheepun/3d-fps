#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

//out vec4 input_fragmentPosition;
out vec2 input_texturePosition;
//out vec4 input_fragmentNormal;
out float input_depth;
out float input_shadowDepth;
out float input_bigShadowDepth;
out vec2 input_shadowMapPosition;
out vec2 input_bigShadowMapPosition;

uniform sampler2D colorTexture;
uniform samplerBuffer grassPositions;
uniform sampler2D shadowMapTexture;

uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;
uniform mat4 lightCameraMatrix;
uniform mat4 lightPerspectiveMatrix;
uniform mat4 bigLightCameraMatrix;
uniform mat4 bigLightPerspectiveMatrix;

uniform float rotation;

uniform float extraCutOff;

uniform float windTime;

void main(){

	input_texturePosition = attribute_textureVertex;

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	vec4 position = texelFetch(grassPositions, gl_InstanceID);

	float angle = rotation;// + position.w;

	mat2 rotationMatrix = mat2(
		cos(angle), -sin(angle),
		sin(angle), cos(angle)
	);

	vertexPosition.xz *= rotationMatrix;

	vertexPosition.xyz += position.xyz;

	float timeOffset = sin(vertexPosition.x / 100.0) + position.w;
	float heightFactor = (1.0 + attribute_vertex.y) / 2.0;
	vertexPosition.x += 0.2 * (sin(windTime + timeOffset)) * heightFactor * heightFactor * heightFactor;

	vec4 lightProjectedVertexPosition = vertexPosition * lightCameraMatrix * lightPerspectiveMatrix;
	input_shadowDepth = 0.5 * lightProjectedVertexPosition.z + 0.5;
	input_shadowMapPosition = (vec2(1.0, 1.0) + lightProjectedVertexPosition.xy) / 2.0;

	vec4 bigLightProjectedVertexPosition = vertexPosition * bigLightCameraMatrix * bigLightPerspectiveMatrix;
	input_bigShadowDepth = 0.5 * bigLightProjectedVertexPosition.z + 0.5;
	input_bigShadowMapPosition = (vec2(1.0, 1.0) + bigLightProjectedVertexPosition.xy) / 2.0;

	gl_Position = vertexPosition * cameraMatrix * perspectiveMatrix;

}

