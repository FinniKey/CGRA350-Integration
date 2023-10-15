#version 330 core

// uniform data
uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;
uniform vec3 uColor;

// mesh data
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;
layout(location = 4) in float aScale;

// model data (this must match the input of the vertex shader)
out VertexData {
	vec3 position;
	vec3 normal;
	vec2 textureCoord;
	vec4 color;

} v_out;

void main() {
	// Apply scaling factor to the vertex position
	vec3 scaledPosition = aPosition * aScale;

	// Transform vertex data to viewspace
	v_out.position = (uModelViewMatrix * vec4(scaledPosition, 1)).xyz;
	v_out.normal = normalize((uModelViewMatrix * vec4(aNormal, 0)).xyz);
	v_out.textureCoord = aTexCoord;
	v_out.color = aColor;


	// Set the screenspace position (needed for converting to fragment data)
	gl_Position = uProjectionMatrix * vec4(v_out.position, 1);
}