import * as MathUtils from './three/math/MathUtils.js';

const epsilon = 0.000000000000001;

//const a = 63.781370;		//椭球长半轴
const a = 6378137.0;		//椭球长半轴
const f_inverse = 298.257223563;			//扁率倒数
const b = a - a / f_inverse;
//const  b = 6356752.314245;			//椭球短半轴

const e = Math.sqrt(a * a - b * b) / a;

function Blh2Xyz(geoPoint) {
    let L = geoPoint.x * MathUtils.DEG2RAD;
    let B = geoPoint.y * MathUtils.DEG2RAD;
    let H = geoPoint.z;

    let N = a / Math.sqrt(1 - e * e * Math.sin(B) * Math.sin(B));
    geoPoint.x = (N + H) * Math.cos(B) * Math.cos(L);
    geoPoint.y = (N + H) * Math.cos(B) * Math.sin(L);
    geoPoint.z = (N * (1 - e * e) + H) * Math.sin(B);
}

export { Blh2Xyz };