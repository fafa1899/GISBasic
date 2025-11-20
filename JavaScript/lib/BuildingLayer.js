import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import { buildingShader } from './BuildingShader.js';

class BuildingLayer {
    constructor(root, filePath) {
        this.root = root;
        this.filePath = filePath;
        this.#CreateBuildingMaterial();
    }

    Load() {
        const loader = new GLTFLoader();
        loader.load(this.filePath, (gltf) => {
            const model = gltf.scene;
            let buildingMesh = new THREE.Mesh(model.children[0].geometry, this.buildingMaterial);
            this.root.add(buildingMesh);           
        }, function (xhr) {
        }, function (error) {
            console.error(error);
        });
    }

    #CreateBuildingMaterial() {
        this.buildingMaterial = new THREE.ShaderMaterial({
            uniforms: {
                "cameraWorldPosition": { value: new THREE.Vector3() },
                "lightSourceDirection": { value: new THREE.Vector3 },
                "lightColor": { value: new THREE.Vector3(1.0, 1.0, 1.0) },
                "ambientStrength": { value: 0.25 },
                "diffuseStrength": { value: 0.5 },
                "shininess": { value: 32.0 },
                "specularStrength": { value: 0.5 },
                "pickupObjectId": { value: 0 },               
            },
            vertexShader: buildingShader.vertexShader,
            fragmentShader: buildingShader.fragmentShader,
            vertexColors: true, // 告诉材质使用顶点颜色
        });
    }

    get BuildingMaterial() {
        return this.buildingMaterial;
    }
}

export default BuildingLayer;