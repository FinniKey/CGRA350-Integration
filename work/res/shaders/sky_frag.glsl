#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform sampler2D uTexture;

void main()
{    
    //FragColor = texture(uTexture, TexCoords.xy);
    FragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow color
}