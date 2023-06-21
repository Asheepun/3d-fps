#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

out vec4 input_fragmentPosition;
out vec2 input_texturePosition;
out vec4 input_fragmentNormal;

uniform sampler2D colorTexture;
uniform samplerBuffer grassPositions;

uniform mat4 perspectiveMatrix;
uniform mat4 cameraMatrix;

uniform float rotation;

uniform float windTime;

void main(){

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	vec4 position = texelFetch(grassPositions, gl_InstanceID);

	float angle = rotation;// + position.w;

	mat2 rotationMatrix = mat2(
		cos(angle), -sin(angle),
		sin(angle), cos(angle)
	);

	//vertexPosition.xyz *= 0.1;

	vertexPosition.xz *= rotationMatrix;

	vertexPosition.xyz += position.xyz;

	//vec2 mapPosition = (vec2(int(gl_InstanceID) % 100, int(int(gl_InstanceID) / 100)) / 100.0) * 1.5;

	//vertexPosition.y += texture2D(heightMap, mapPosition).x * 100.0;
	//vertexPosition.y += 0.5;
	//vertexPosition.y += 10.0;

	//vertexPosition.x += mapPosition.x * 100.0;
	//vertexPosition.z += mapPosition.y * 100.0;

	float timeOffset = sin(vertexPosition.x / 100.0) + position.w;
	float heightFactor = (1.0 + attribute_vertex.y) / 2.0;
	vertexPosition.x += 0.2 * (sin(windTime + timeOffset)) * heightFactor * heightFactor * heightFactor;

	input_fragmentPosition = vertexPosition;
	input_texturePosition = attribute_textureVertex;
	input_fragmentNormal = vertexNormal;

	vertexPosition *= cameraMatrix * perspectiveMatrix;

	gl_Position = vertexPosition;

	gl_Position.z = 0.0;

}

