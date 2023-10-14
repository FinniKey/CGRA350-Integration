#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// model data (this must match the input of the vertex shader)
out VertexData {
    vec3 position;
	vec3 normal;
    vec2 textureCoord;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} v_out;

// uniform data
uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;
uniform vec3 uColor;


uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
	// transform vertex data to viewspace
	//v_out.position = (uModelViewMatrix * vec4(aPosition, 1)).xyz;
	//v_out.normal = normalize((uModelViewMatrix * vec4(aNormal, 0)).xyz);

	// set the screenspace position (needed for converting to fragment data)
	gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPosition, 1);
    v_out.position = (uModelViewMatrix * vec4(aPosition, 1)).xyz;
    v_out.textureCoord = aTexCoord;
    
    vec3 T   = normalize(mat3(uModelViewMatrix) * aTangent);
    vec3 B   = normalize(mat3(uModelViewMatrix) * aBitangent);
    vec3 N   = normalize(mat3(uModelViewMatrix) * aNormal);
    mat3 TBN = transpose(mat3(T, B, N));

    v_out.TangentLightPos = TBN * lightPos;
    v_out.TangentViewPos  = TBN * viewPos;
    v_out.TangentFragPos  = TBN * v_out.position;
}