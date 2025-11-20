//着色器
const buildingShader = {
    vertexShader: `  
        out vec3 worldPosition;
        out vec3 worldNormal;	       
        out vec4 idColor;
        void main() { 
            gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 ); 
            worldPosition = (modelMatrix * vec4( position, 1.0 )).xyz;
            mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
            worldNormal = normalize(normalMatrix * normal);      
            idColor = color;      
        }`
    ,

    fragmentShader: `	
        uniform vec3 cameraWorldPosition;	
        uniform vec3 lightSourceDirection;
        uniform vec3 lightColor;
        uniform float ambientStrength;
        uniform float diffuseStrength;
        uniform float shininess;
        uniform float specularStrength;        
        uniform float pickupObjectId;            
        in vec3 worldPosition;	
        in vec3 worldNormal;	
        in vec4 idColor;	      
        
        //计算要素的ObjectId
        float Color2Id(vec4 idColor){
            float value = idColor.x * 255.0;
            value += idColor.y * 255.0 * 256.0;
            value += idColor.z * 255.0 * 256.0 * 256.0;
            value += idColor.w * 255.0 * 256.0 * 256.0 * 256.0;
            return value;
        }

        void main() {           
            float objectId = Color2Id(idColor);
            vec3 baseColor = vec3(1.0, 1.0, 1.0);
            if(abs(pickupObjectId - objectId) < 0.5){
                baseColor = vec3(1.0, 0.0, 0.0);
            }

            vec3 lightDirection = normalize(lightSourceDirection);          
            lightDirection = normalize(lightDirection);
            vec3 normal = normalize(worldNormal);

            //环境反射
            vec3 ambient = ambientStrength * lightColor;
            //漫反射
            vec3 diffuse = diffuseStrength * lightColor * dot(lightDirection, normal);
            //镜面反射
            vec3 viewDirection = normalize(cameraWorldPosition - worldPosition);				            		
            vec3 halfwayDirection = normalize(viewDirection + lightDirection);            
            vec3 specular = specularStrength * pow(max(dot(normal, halfwayDirection), 0.0), shininess) * lightColor;        
                    
            vec3 resultColor = (ambient + diffuse + specular) * baseColor;
            gl_FragColor = vec4(resultColor, 1.0); 
        }`
};

export { buildingShader };