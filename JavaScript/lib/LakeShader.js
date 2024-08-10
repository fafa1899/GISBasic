//着色器
const lakeShader = {
    vertexShader: `   			
        uniform float elapsedTime;
        uniform mat4 reflectionViewMatrix;
        uniform mat4 reflectionprojectionMatrix;
        uniform vec2 waveDirection;
        uniform float waveLength; 
        uniform float waveSpeed;
        out vec3 worldPosition;
        out vec3 worldNormal;	
        out vec2 bumpUv;		
        out vec4 reflectionClip;	
        void main() { 
            gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 ); 
            worldPosition = (modelMatrix * vec4( position, 1.0 )).xyz;
            worldNormal = normalize(normalMatrix * normal);
            reflectionClip = reflectionprojectionMatrix * reflectionViewMatrix * modelMatrix * vec4( position, 1.0 ); 

            // moving the water       
            bumpUv = uv/waveLength + elapsedTime * waveSpeed * waveDirection;            
        }`
    ,

    fragmentShader: `		
        uniform vec3 lightSourceDirection;
        uniform vec3 cameraWorldPosition;
        uniform sampler2D reflectionTexture; 
        uniform sampler2D bumpMap; 
        uniform float waveHeight;   
        uniform float shininess;
        uniform float specularStrength;
        uniform float specularPerturbation;
        uniform vec3 specularLightColor;
        uniform float waterAlpha;
        in vec3 worldPosition;	
        in vec3 worldNormal;	
        in vec2 bumpUv;		
        in vec4 reflectionClip;	        	
        void main() {    
            // perturbation of the color				
            vec4 bumpColor = texture(bumpMap, bumpUv);				
            vec2 s = vec2(0.5f, 0.5f); 
            vec2 perturbation = waveHeight * (bumpColor.rg - s);
                                    
            vec3 reflectionCoord = (reflectionClip.xyz/reflectionClip.w)/2.0 + 0.5;
            vec2 reflectedUv = reflectionCoord.xy + perturbation;
            vec3 resultColor = texture(reflectionTexture, reflectedUv).xyz; 	
                                                
            vec3 viewDirection = normalize(cameraWorldPosition - worldPosition);				            		
            vec3 halfwayDirection = normalize(viewDirection + lightSourceDirection + vec3(perturbation.x*specularPerturbation,perturbation.y*specularPerturbation,0));
            
            float spec = pow(max(dot(worldNormal, halfwayDirection), 0.0), shininess);
            vec3 specular = specularLightColor * spec;	
            resultColor = resultColor + specularStrength * specular;					

            gl_FragColor = vec4(resultColor, waterAlpha); 
        }`
};

export { lakeShader };