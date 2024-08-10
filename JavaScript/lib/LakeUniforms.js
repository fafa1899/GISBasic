import * as THREE from 'three';

const lakeUniforms = {    
    "waveDirection": { value: new THREE.Vector2(0, 1) },
    "waveLength": { value: 0.5 },
    "waveSpeed": { value: 0.06 },
    "waveHeight": { value: 0.1 },
    "shininess": { value: 8.0 },
    "specularStrength": { value: 0.6 },
    "specularPerturbation": { value: 0.12 },
    "specularLightColor": { value: new THREE.Color(1.0, 1.0, 1.0) },
    "waterAlpha": { value: 0.5 }
}

export { lakeUniforms }