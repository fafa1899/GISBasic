import { GUI } from 'three/examples/jsm/libs/lil-gui.module.min.js';
import { lakeUniforms } from './LakeUniforms.js';

const lakeMaterialGui = new GUI();
lakeMaterialGui.title("湖面效果设置");

const waveEffect = lakeMaterialGui.addFolder("水面波纹效果");
const waveDirection = waveEffect.addFolder("波纹移动方向");
waveDirection.add(lakeUniforms.waveDirection.value, 'x');
waveDirection.add(lakeUniforms.waveDirection.value, 'y');
waveEffect.add(lakeUniforms.waveLength, "value", 0, 1, 0.1).name("波纹长度");
waveEffect.add(lakeUniforms.waveSpeed, "value").name("波纹速度");
waveEffect.add(lakeUniforms.waveHeight, "value").name("波纹高度");

const specularEffect = lakeMaterialGui.addFolder("镜面反射效果");
specularEffect.addColor(lakeUniforms.specularLightColor, "value").name("镜面光颜色");
specularEffect.add(lakeUniforms.specularStrength, "value", 0, 1, 0.1).name("镜面强度");
specularEffect.add(lakeUniforms.shininess, "value", 1, 100, 1).name("镜面反光度");
specularEffect.add(lakeUniforms.specularPerturbation, "value").name("镜面扰动系数");

lakeMaterialGui.add(lakeUniforms.waterAlpha, "value", 0, 1, 0.1).name("水体透明度");

export { lakeMaterialGui };