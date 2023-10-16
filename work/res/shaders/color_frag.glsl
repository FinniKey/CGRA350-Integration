#version 330 core

// Input from the vertex shader
in VertexData {
    vec3 position;
	vec3 normal;
    vec2 textureCoord;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
} v_in;

// Uniform data
uniform sampler2D uTexture;
uniform sampler2D uNormal;
uniform sampler2D uHeight;

uniform float heightScale;
uniform float tilingScale;
uniform float POMmaxLayers;

out vec4 fragColor;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir);
float perlinNoise(vec2 p);

uniform float uDepthMode;
uniform sampler2D depthMap;
uniform vec2 mapSize;
uniform int uSearchRegion;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    // number of depth layers
    const float minLayers = 32;
    float maxLayers = POMmaxLayers;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir))); 

    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;

    // depth of current layer
    float currentLayerDepth = 0.0;

    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = 1.0 - texture(uHeight, currentTexCoords).r; // Invert the depth map value

    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;

        // get depth map value at current texture coordinates and invert it
        currentDepthMapValue = 1.0 - texture(uHeight, currentTexCoords).r;

        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0 - texture(uHeight, prevTexCoords).r - currentLayerDepth + layerDepth; // Invert the depth map value

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}


float shadowCalc(vec2 texCoord, vec3 lightDir)
{
    if ( lightDir.z >= 0.0 )
        return 0.0;

    float minLayers = 0;
    float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), lightDir)));

    vec2 currentTexCoords = texCoord;
    float currentDepthMapValue = texture(uHeight, currentTexCoords).r;
    float currentLayerDepth = currentDepthMapValue;

    float layerDepth = 1.0 / numLayers;
    vec2 P = lightDir.xy / lightDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    while (currentLayerDepth <= currentDepthMapValue && currentLayerDepth > 0.0)
    {
        currentTexCoords += deltaTexCoords;
        currentDepthMapValue = texture(uHeight, currentTexCoords).r;
        currentLayerDepth -= layerDepth;
    }

    float r = currentLayerDepth > currentDepthMapValue ? 0.0 : 1.0;
    return r;
}

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
		fragColor = vec4(vec3(gl_FragCoord.z), 1.0);
	}
    else {

        vec3 viewDir = normalize(v_in.TangentViewPos - v_in.TangentFragPos);

        vec2 scaledCoords = fract(v_in.textureCoord * tilingScale);
        vec2 texCoords = scaledCoords;


        texCoords = ParallaxMapping(scaledCoords, viewDir);

          // Add mirroring at the edges of tiles
        if (texCoords.x > 1.0) {
        texCoords.x = 2.0 - texCoords.x;
        } else if (texCoords.x < 0.0) {
        texCoords.x = -texCoords.x;
        }

        if (texCoords.y > 1.0) {
        texCoords.y = 2.0 - texCoords.y;
        } else if (texCoords.y < 0.0) {
            texCoords.y = -texCoords.y;
        }

        vec2 ddx = dFdx(texCoords); // apparently removes ugly aliasing
	    vec2 ddy = dFdy(texCoords);

        vec3 normal = textureGrad(uNormal, texCoords, ddx, ddy).rgb;
        normal = normalize(normal * 2.0 - 1.0);

        // get diffuse color
        vec3 color = textureGrad(uTexture, texCoords, ddx, ddy).rgb;

        // ambient
        vec3 ambient = 0.25 * color;

        // diffuse
        vec3 lightDir = normalize(v_in.TangentLightPos - v_in.TangentFragPos);
        float diff = max(dot(lightDir, normal), 0.0);

        //float shadow = diff > 0.0 ? shadowCalc(texCoords, lightDir) : 0.0;

        float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005); 
        float shadow = ShadowCalculation(v_in.FragPosLightSpace, bias);
    
        vec3 diffuse = diff * color;

        // specular    
        vec3 reflectDir = reflect(-lightDir, normal);
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

        vec3 specular = vec3(0.2) * spec;

        vec3 finalColor = ambient + (1.0 - shadow) * (diffuse + specular);

        fragColor = vec4(finalColor, 1.0);

        // draw shadow map onto object
		//float depthVal = texture(depthMap, v_in.textureCoord).r;
		//fragColor = vec4(vec3(depthVal), 1.0);
    }
}