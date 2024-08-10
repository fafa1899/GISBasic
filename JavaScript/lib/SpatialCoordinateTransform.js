import * as THREE from 'three';
import proj4 from 'proj4';

class SpatialCoordinateTransform {
    constructor(centerX, centerY) {
        const wgs84Gcs = "+proj=longlat +datum=WGS84 +no_defs +type=crs";
        const wgs84Ecef = "+proj=geocent +datum=WGS84 +units=m +no_defs +type=crs";
        const utm11Pcs = "+proj=utm +zone=11 +datum=WGS84 +units=m +no_defs +type=crs";
        this.utm11Pcs2Wgs84Ecef = proj4(utm11Pcs, wgs84Ecef);
        this.utm11Pcs2Wgs84Gcs = proj4(utm11Pcs, wgs84Gcs);
        this.wgs84Gcs2Wgs84Ecef = proj4(wgs84Gcs, wgs84Ecef);
        this.wgs84Gcs2Utm11Pcs = proj4(wgs84Gcs, utm11Pcs);
        this.wgs84Ecef2Wgs84Gcs = proj4(wgs84Ecef, wgs84Gcs);

        this.matEcef2Enu = new THREE.Matrix4();//地心坐标转ENU坐标矩阵
        this.matEnu2Ecef = new THREE.Matrix4();//ENU坐标矩阵转地心坐标       
        const topocentricOrigin = this.Utm2Gcs(new THREE.Vector3(centerX, centerY, 0));
        this.#ComputeEcef2Enu(topocentricOrigin);
        this.#ComputeEnu2Ecef(topocentricOrigin);
    }

    //计算Ecef到Enu的转换
    #ComputeEcef2Enu(topocentricOrigin) {
        let radianLon = topocentricOrigin.x * THREE.MathUtils.DEG2RAD;
        let radianLat = topocentricOrigin.y * THREE.MathUtils.DEG2RAD;

        let sinLon = Math.sin(radianLon);
        let cosLon = Math.cos(radianLon);
        let sinLat = Math.sin(radianLat);
        let cosLat = Math.cos(radianLat);

        this.matEcef2Enu.set(-sinLon, cosLon, 0, 0, -sinLat * cosLon, -sinLat * sinLon,
            cosLat, 0, cosLat * cosLon, cosLat * sinLon, sinLat, 0, 0, 0, 0, 1);

        let coord = this.wgs84Gcs2Wgs84Ecef.forward([topocentricOrigin.x, topocentricOrigin.y, topocentricOrigin.z]);
        let translatMat = new THREE.Matrix4(1, 0, 0, -coord[0], 0, 1, 0, -coord[1], 0, 0, 1, -coord[2], 0, 0, 0, 1);

        this.matEcef2Enu.multiply(translatMat);
    }

    #ComputeEnu2Ecef(topocentricOrigin) {
        let coord = this.wgs84Gcs2Wgs84Ecef.forward([topocentricOrigin.x, topocentricOrigin.y, topocentricOrigin.z]);

        let radianLon = topocentricOrigin.x * THREE.MathUtils.DEG2RAD;
        let radianLat = topocentricOrigin.y * THREE.MathUtils.DEG2RAD;
        let sinLon = Math.sin(radianLon);
        let cosLon = Math.cos(radianLon);
        let sinLat = Math.sin(radianLat);
        let cosLat = Math.cos(radianLat);

        this.matEnu2Ecef.set(-sinLon, -sinLat * cosLon, cosLat * cosLon, coord[0], cosLon,
            -sinLat * sinLon, cosLat * sinLon, coord[1], 0, cosLat, sinLat, coord[2], 0, 0, 0, 1);
    }

    Utm2Ecef(utm) {
        let coord = this.utm11Pcs2Wgs84Ecef.forward([utm.x, utm.y, utm.z]); //UTM坐标转地心直角坐标
        return new THREE.Vector3(coord[0], coord[1], coord[2]);
    }

    Utm2Gcs(utm) {
        let coord = this.utm11Pcs2Wgs84Gcs.forward([utm.x, utm.y, utm.z]);
        return new THREE.Vector3(coord[0], coord[1], coord[2]);
    }

    Utm2Enu(utm) {
        const result = spatialCoordinateTransform.Utm2Ecef(new THREE.Vector3(utm.x, utm.y, utm.z));
        result.applyMatrix4(this.matEcef2Enu);
        return result;
    }

    Gcs2Ecef(gcs) {
        const ecef = this.wgs84Gcs2Wgs84Ecef.forward([gcs.x, gcs.y, gcs.z]);
        return new THREE.Vector3(ecef[0], ecef[1], ecef[2]);
    }

    Gcs2Utm(gcs) {   
        let coord = this.wgs84Gcs2Utm11Pcs.forward([gcs.x, gcs.y, gcs.z]);
        return new THREE.Vector3(coord[0], coord[1], coord[2]);
    }

    Ecef2Gcs(ecef) {
        const gcs = this.wgs84Ecef2Wgs84Gcs.forward([ecef.x, ecef.y, ecef.z]);
        return new THREE.Vector3(gcs[0], gcs[1], gcs[2]);
    }

    Enu2Ecef(enu) {
        const result = enu.clone();
        result.applyMatrix4(this.matEnu2Ecef);
        return result;
    }

    Ecef2Enu(ecef) {
        const result = ecef.clone();
        result.applyMatrix4(this.matEcef2Enu);
        return result;
    }

    Gcs2Enu(gcs) {
        const result = this.Gcs2Ecef(gcs);
        result.applyMatrix4(this.matEcef2Enu);
        return result;
    }

    Enu2Gcs(enu) {
        const ecef = this.Enu2Ecef(enu);
        const camreaWgs84Coord = this.wgs84Ecef2Wgs84Gcs.forward([ecef.x, ecef.y, ecef.z]);
        ecef.x = camreaWgs84Coord[0];
        ecef.y = camreaWgs84Coord[1];
        ecef.z = camreaWgs84Coord[2];
        return ecef;
    }

    get MatEnu2Ecef() {
        return this.matEnu2Ecef;
    }

}

const spatialCoordinateTransform = new SpatialCoordinateTransform(377967.5, 3774907.5);

export { spatialCoordinateTransform };