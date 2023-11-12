"use strict"

var viewer = new Cesium.Viewer('cesiumContainer');

createModel('./wh.gltf');

function createModel(url) {
    viewer.entities.removeAll();

    var position = Cesium.Cartesian3.fromDegrees(-77.036252, 38.897557, 0);
    var heading = Cesium.Math.toRadians(0);
    var pitch = Cesium.Math.toRadians(0);
    var roll = 0;
    var hpr = new Cesium.HeadingPitchRoll(heading, pitch, roll);
    var orientation = Cesium.Transforms.headingPitchRollQuaternion(position, hpr);

    var entity = viewer.entities.add({
        name: url,
        position: position,
        orientation: orientation,
        model: {
            uri: url         
        }
    });
    //viewer.trackedEntity = entity;

    viewer.camera.setView({
        destination: Cesium.Cartesian3.fromDegrees(-77.0365, 38.8977, 1000),
        orientation: {
            heading: Cesium.Math.toRadians(0.0),
            pitch: Cesium.Math.toRadians(-90.0),
            roll: Cesium.Math.toRadians(0.0)
        }
    });
}
