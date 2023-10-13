#version 330 core

// uniform data
uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;
uniform vec3 uColor;

uniform float uDepthMode;
uniform mat4 depthMVP;

uniform vec3 lightPos;

// viewspace data (this must match the output of the fragment shader)
in VertexData {
	vec3 position;
	vec3 normal;
	vec2 textureCoord;
	vec4 FragPosLightSpace;
} f_in;

// framebuffer output
out vec4 fb_color;

uniform sampler2D depthMap;
uniform vec2 mapSize;

uniform mat4 uBiasMatrix;
uniform mat4 uModelTransformation;

uniform int uSearchRegion;

float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

	float xOffset = 1.0 / mapSize.x;
	float yOffset = 1.0 / mapSize.y;
	float factor = 0.0;

	for (int y = -uSearchRegion; y <= uSearchRegion; y++) {
		for (int x = -uSearchRegion; x <= uSearchRegion; x++) {
			vec2 Offsets = vec2(x * xOffset, y * yOffset);

			float projOffsetX = projCoords.x + Offsets.x;
			float projOffsetY = projCoords.y + Offsets.y;

			//if ( projOffsetX > 0.0 && projOffsetX < 1.0 && projOffsetY > 0.0 && projOffsetY < 1.0 ) {

				vec3 UVC = vec3(projCoords.xy + Offsets, projCoords.z + bias);
				factor += texture(depthMap, UVC.xy).r;
			//}
		}
	}


    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(depthMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

	float singleLine = (uSearchRegion * 2) + 1;

	shadow = 1.0 - (factor / (singleLine * singleLine));

	if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}  

void main() {

	if (uDepthMode == 1) {
		//float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
		fb_color = vec4(vec3(gl_FragCoord.z), 1.0);
	}
	else {
		
		// draw shadow map onto object
		//float depthVal = texture(depthMap, f_in.textureCoord).r;
		//fb_color = vec4(vec3(depthVal), 1.0);


		// calculate lighting (hack)
		//vec3 eye = normalize(-f_in.position);
		//float light = abs(dot(normalize(f_in.normal), eye));
		//vec3 color = mix(uColor / 4, uColor, light);

		// output to the frambuffer
		//fb_color = vec4(color, 1);
		
		
		


		vec3 color = uColor;
		vec3 normal = normalize(f_in.normal);        
		vec3 lightColor = vec3(1.0);
        // ambient
        vec3 ambient = 0.15 * lightColor;
        // diffuse
		vec3 lightDir = normalize(lightPos - f_in.position);        
		float diff = max(dot(lightDir, normal), 0.0);
        vec3 diffuse = diff * lightColor;
        // specular
        vec3 viewDir = lightDir;
        float spec = 0.0;
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
        vec3 specular = spec * lightColor;  
		specular = vec3(0);
        // calculate shadow
        float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005); 
        float shadow = ShadowCalculation(f_in.FragPosLightSpace, bias);       
        vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
        fb_color = vec4(lighting, 1.0);
	}
	
}