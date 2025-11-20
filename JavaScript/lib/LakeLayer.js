import * as THREE from 'three';
//import { lakeMaterial } from './LakeMaterial.js';
import { spatialCoordinateTransform } from './SpatialCoordinateTransform.js'
import { RayEllipsoid } from './RayEllipsoid.js';
import { lakeShader } from './LakeShader.js';
import { lakeUniforms } from './LakeUniforms.js';

class LakeLayer {
    constructor(root, filePath, waterHeight) {
        this.root = root;
        this.filePath = filePath;
        this.waterHeight = waterHeight;
        this.reflectCamera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 20000); // 相机
        this.lakeMaterial = new THREE.ShaderMaterial({
            uniforms: {
                "cameraWorldPosition": { value: new THREE.Vector3() },
                "reflectionTexture": { value: null },
                "bumpMap": { value: null },
                "elapsedTime": { value: 0.0 },
                "reflectionViewMatrix": { value: new THREE.Matrix4 },
                "reflectionprojectionMatrix": { value: new THREE.Matrix4 },
                "lightSourceDirection": { value: new THREE.Vector3 },
                "waveDirection": lakeUniforms.waveDirection,
                "waveLength": lakeUniforms.waveLength,
                "waveSpeed": lakeUniforms.waveSpeed,
                "waveHeight": lakeUniforms.waveHeight,
                "shininess": lakeUniforms.shininess,
                "specularStrength": lakeUniforms.specularStrength,
                "specularPerturbation": lakeUniforms.specularPerturbation,
                "specularLightColor": lakeUniforms.specularLightColor,
                "waterAlpha": lakeUniforms.waterAlpha
            },
            vertexShader: lakeShader.vertexShader,
            fragmentShader: lakeShader.fragmentShader,
            transparent: true // 开启透明			
        });
    }

    LoadLakeModel() {
        let httpRequest = new XMLHttpRequest();
        httpRequest.open("GET", this.filePath, true);
        httpRequest.onreadystatechange = () => {
            if (httpRequest.readyState == 4 && httpRequest.status == 200) {
                let lakeMesh = this.#CreateLakeMesh(httpRequest.responseText);
                if (lakeMesh) {
                    this.root.add(lakeMesh);
                }
            }
        };
        httpRequest.send();
    }

    #CreateLakeMesh(result) {
        let stringlines = result.split("\r\n");
        if (stringlines.length <= 10) {
            return null;
        }

        let subline = stringlines[3].split(/[ ]+/);
        let vertexCount = parseInt(subline[2]);
        const positions = new Float32Array(vertexCount * 3);
        const normals = new Float32Array(vertexCount * 3);
        const uvs = new Float32Array(vertexCount * 2);
        for (let vi = 0; vi < vertexCount; vi++) {
            let attributeStrings = stringlines[15 + vi].split(/[ ]+/);
            positions[vi * 3] = parseFloat(attributeStrings[0]);
            positions[vi * 3 + 1] = parseFloat(attributeStrings[1]);
            positions[vi * 3 + 2] = parseFloat(attributeStrings[2]);
            normals[vi * 3] = parseFloat(attributeStrings[3]);
            normals[vi * 3 + 1] = parseFloat(attributeStrings[4]);
            normals[vi * 3 + 2] = parseFloat(attributeStrings[5]);
            uvs[vi * 2] = parseFloat(attributeStrings[6]);
            uvs[vi * 2 + 1] = parseFloat(attributeStrings[7]);
        }

        subline = stringlines[12].split(/[ ]+/);
        let faceNum = parseInt(subline[2]);
        const indices = new Array(faceNum * 3);
        for (let fi = 0; fi < faceNum; fi++) {
            let indiceStrings = stringlines[15 + vertexCount + fi].split(/[ ]+/);
            indices[fi * 3] = parseInt(indiceStrings[1]);
            indices[fi * 3 + 1] = parseInt(indiceStrings[2]);
            indices[fi * 3 + 2] = parseInt(indiceStrings[3]);
        }

        const lakeGeometry = new THREE.BufferGeometry();
        lakeGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
        lakeGeometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
        lakeGeometry.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
        lakeGeometry.setIndex(indices);

        //加载凹凸贴图并更新材质
        const loader = new THREE.TextureLoader();
        loader.load(
            'public/gradient_map.jpg',
            (texture) => {
                texture.wrapS = THREE.MirroredRepeatWrapping;
                texture.wrapT = THREE.MirroredRepeatWrapping;
                texture.magFilter = THREE.LinearFilter;
                texture.minFilter = THREE.LinearMipmapLinearFilter;
                texture.needsUpdate = true;

                this.lakeMaterial.uniforms.bumpMap.value = texture;
                this.lakeMaterial.needsUpdate = true;
            },
            undefined,
            function (err) {
                console.error('An error happened:' + err);
            }
        );

        let lakeMesh = new THREE.Mesh(lakeGeometry, this.lakeMaterial);
        return lakeMesh;
    }



    SetReflectData(camera, texture) {
        const rayOrigin = spatialCoordinateTransform.Enu2Ecef(new THREE.Vector3(camera.position.x, -camera.position.z, camera.position.y));
        const camreaWgs84Coord = spatialCoordinateTransform.Ecef2Gcs(new THREE.Vector3(rayOrigin.x, rayOrigin.y, rayOrigin.z));
        const heightOffset = (camreaWgs84Coord.z - this.waterHeight);
        const reflectHeight = this.waterHeight - heightOffset;
        const reflectCameraPosition = spatialCoordinateTransform.Gcs2Enu(new THREE.Vector3(camreaWgs84Coord.x, camreaWgs84Coord.y, reflectHeight));
        this.reflectCamera.position.set(reflectCameraPosition.x, reflectCameraPosition.z, -reflectCameraPosition.y);

        let direction = new THREE.Vector3();
        camera.getWorldDirection(direction);
        let rayDirection = new THREE.Vector3(direction.x, -direction.z, direction.y);
        let roation = new THREE.Matrix3();
        roation.setFromMatrix4(spatialCoordinateTransform.MatEnu2Ecef);
        rayDirection.applyMatrix3(roation);
        rayDirection.normalize();

        let halfAxis = new THREE.Vector3(6378137.0 + this.waterHeight, 6378137.0 + this.waterHeight, 6356752.3142451793 + this.waterHeight);
        let intersections = RayEllipsoid(halfAxis, rayOrigin, rayDirection);

        if (intersections && intersections.length > 0) {
            const enuIntersection = spatialCoordinateTransform.Ecef2Enu(intersections[0]);
            this.reflectCamera.lookAt(new THREE.Vector3(enuIntersection.x, enuIntersection.z, -enuIntersection.y));
        }

        this.reflectCamera.up.set(0, 1, 0);
        this.reflectCamera.updateMatrixWorld(true);

        this.lakeMaterial.uniforms.cameraWorldPosition.value = camera.position;
        this.lakeMaterial.uniforms.reflectionTexture.value = texture;
        this.lakeMaterial.uniforms.reflectionViewMatrix.value = this.reflectCamera.matrixWorldInverse;
        this.lakeMaterial.uniforms.reflectionprojectionMatrix.value = this.reflectCamera.projectionMatrix;
    }

    get ReflectCamera() {
        return this.reflectCamera;
    }

    get LakeMaterial() {
        return this.lakeMaterial;
    }

}


export default LakeLayer;