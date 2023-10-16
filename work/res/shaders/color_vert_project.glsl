#version 330 core

// uniform data
uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;
uniform vec3 uColor;

// mesh data
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

// depth map stuff
uniform mat4 depthMVP;
uniform float uDepthMode;

// model data (this must match the input of the vertex shader)
out VertexData {
	vec3 position;
	vec3 normal;
	vec2 textureCoord;
	vec4 FragPosLightSpace;
} v_out;

void main() {

	if (uDepthMode == 1) {
		
        gl_Position = depthMVP * vec4(aPosition, 1);
    }
    else {

		// transform vertex data to viewspace
		v_out.position = (uModelViewMatrix * vec4(aPosition, 1)).xyz;
		v_out.normal = normalize((uModelViewMatrix * vec4(aNormal, 0)).xyz);
		v_out.textureCoord = aTexCoord;

		v_out.FragPosLightSpace = depthMVP * vec4(aPosition, 1);

		// set the screenspace position (needed for converting to fragment data)
		gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition, 1);
	}
}