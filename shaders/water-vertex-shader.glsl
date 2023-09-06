#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

out vec4 input_fragmentPosition;
out vec3 input_normal;
out vec2 input_textureCoord;

uniform float scale;
uniform mat4 rotationMatrix;
uniform mat4 scalingMatrix;
uniform mat4 translationMatrix;
uniform mat4 viewMatrix;

uniform float inputT;

vec3 waveFunction(vec2 v){

	v *= scale;

	float height = (sin(v.x * scale + inputT) + sin(v.y * scale + inputT * 2.0)) * 0.5;
	float distortionX = 0.0;
	float distortionY = 0.0;
	//float distortionX = sin(v.x * scale + inputT * 3.0) + sin(v.y * scale + inputT) * 0.1;
	//float distortionY = sin(v.x * scale + inputT * 2.0) + sin(v.y * scale + inputT * 0.5) * 0.1;

	float tolerance = 0.01;

	if(v.y < tolerance
	|| v.x < tolerance
	|| v.x > scale - tolerance
	|| v.y > scale - tolerance){
		height = 0.0;
		distortionX = 0.0;
		distortionY = 0.0;
	}

	return vec3(v.x + distortionX, height, v.y + distortionY);
}

void main(){

	vec3 point1 = waveFunction(attribute_vertex.xy);
	vec3 point2 = waveFunction(vec2(attribute_vertex.z, attribute_normalVertex.x));
	vec3 point3 = waveFunction(attribute_normalVertex.yz);

	//point2 = vec3(0.0);
	//point3 = vec3(1.0, 0.0, 0.0);

	input_textureCoord = attribute_textureVertex;

	//vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);
	//vec4 vertexNormal = vec4(attribute_normalVertex.xyz, 1.0);

	vec4 vertexPosition = vec4(point1, 1.0);

	//vertexPosition *= rotationMatrix;

	//input_fragmentPosition = vertexPosition;

	//vertexPosition *= scalingMatrix;
	vertexPosition.y = point1.y;

	vertexPosition *= translationMatrix;

	input_normal = normalize(cross(point1 - point2, point1 - point3));

	gl_Position = vertexPosition * viewMatrix;

}

