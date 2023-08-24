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

	//input_fragmentPosition = vertexPosition;
	//input_fragmentNormal = vertexNormal;

	vec4 lightRelativeVertexPosition = vertexPosition * lightCameraMatrix;
	vec2 shadowMapVertexPosition = (lightRelativeVertexPosition * lightPerspectiveMatrix).xy;

	input_shadowDepth = lightRelativeVertexPosition.z / 100.0;
	input_shadowMapPosition = (vec2(1.0, 1.0) + shadowMapVertexPosition) / 2.0;

	vec4 bigLightRelativeVertexPosition = vertexPosition * bigLightCameraMatrix;
	vec2 bigShadowMapVertexPosition = (bigLightRelativeVertexPosition * bigLightPerspectiveMatrix).xy;

	input_bigShadowDepth = bigLightRelativeVertexPosition.z / 100.0;
	input_bigShadowMapPosition = (vec2(1.0, 1.0) + bigShadowMapVertexPosition) / 2.0;

	vertexPosition *= cameraMatrix;

	input_depth = vertexPosition.z / 100.0;

	vertexPosition *= perspectiveMatrix;

	gl_Position = vertexPosition;

	gl_Position.z = 0.0;

	//gl_Position.z = input_depth;

	//gl_Position.z = 2.0;

}

