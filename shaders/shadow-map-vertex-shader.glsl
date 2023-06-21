#version 330 core
layout (location = 0) in vec3 attribute_vertex;
layout (location = 1) in vec2 attribute_textureVertex;
layout (location = 2) in vec3 attribute_normalVertex;

out float depth;

uniform samplerBuffer voxelPositionsTextureBuffer;

uniform mat4 cameraMatrix;
uniform mat4 perspectiveMatrix;

uniform float voxelScale;

uniform float shadowMapScale;

void main(){

	vec4 modelPosition = texelFetch(voxelPositionsTextureBuffer, gl_InstanceID);

	vec4 vertexPosition = vec4(attribute_vertex.xyz, 1.0);

	vec4 projectedPosition = vertexPosition;

	projectedPosition.xyz *= voxelScale;
	projectedPosition += modelPosition;
		
	projectedPosition *= cameraMatrix;
	
	depth = projectedPosition.z / 100.0;

	projectedPosition.z = 0.0;
	projectedPosition.w = shadowMapScale;

	gl_Position = projectedPosition;

}

