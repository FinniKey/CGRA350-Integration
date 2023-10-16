#version 330 core

// uniform data
uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;
uniform vec3 uColor;

uniform sampler2D fireTexture;

// viewspace data (this must match the output of the fragment shader)
in VertexData {
	vec3 position;
	vec3 normal;
	vec2 textureCoord;
	vec4 color;

} f_in;

// framebuffer output
out vec4 fb_color;

void main() {
	// calculate lighting (hack)
	vec3 eye = normalize(-f_in.position);
	float light = abs(dot(normalize(f_in.normal), eye));
	

	// output to the frambuffer
	fb_color = texture(fireTexture,gl_PointCoord) *f_in.color;
	fb_color.a = f_in.color.a;
}