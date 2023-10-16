#version 330 core

// uniform data
uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;
uniform vec3 uColor;

// depth stuff
uniform float uDepthMode;

// viewspace data (this must match the output of the fragment shader)
in VertexData {
	vec3 position;
	vec3 normal;
	vec2 textureCoord;
} f_in;

// framebuffer output
out vec4 fb_color;

void main() {
	if (uDepthMode == 1) {
		fb_color = vec4(vec3(gl_FragCoord.z), 1.0);
	}
	else {

		// calculate lighting (hack)
		vec3 eye = normalize(-f_in.position);
		float light = abs(dot(normalize(f_in.normal), eye));
		vec3 color = mix(uColor / 4, uColor, light);

		// output to the frambuffer
		fb_color = vec4(color, 1);
	}
}