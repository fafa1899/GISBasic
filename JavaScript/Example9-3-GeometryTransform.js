import * as THREE from 'three';

'use strict';

//函数立即执行，避免污染全局作用域
(function () {
	var scene = new THREE.Scene(); //场景

	//相机
	var camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 1000);
	camera.position.set(0, 0, 100);   //相机的位置
	camera.up.set(0, 1, 0);         //相机以哪个方向为上方
	camera.lookAt(new THREE.Vector3(1, 2, 3));          //相机看向哪个坐标

	//渲染器
	var renderer = new THREE.WebGLRenderer({
		antialias: true,     //抗锯齿
	});
	renderer.setClearColor(new THREE.Color(0x000000));
	renderer.setPixelRatio(window.devicePixelRatio);//开启HiDPI设置
	renderer.setSize(window.innerWidth, window.innerHeight);
	document.body.appendChild(renderer.domElement);	//add the output of the renderer to the html element

	//着色器
	const customShader = {
		uniforms: {
			"sw": { type: 'b', value: false },
			"mvpMatrix": { type: 'm4', value: new THREE.Matrix4() }
		},

		vertexShader: `    
			uniform mat4 mvpMatrix;
			uniform bool sw;
			void main() { 
				if(sw) {
					gl_Position = mvpMatrix * vec4( position, 1.0 );  
				}else{
					gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 ); 
				}       
			}`
		,

		fragmentShader: `   
			uniform bool sw; 
			void main() {    
				if(sw) {
					gl_FragColor = vec4(0.556, 0.0, 0.0, 1.0); 
				}else {
					gl_FragColor = vec4(0.556, 0.8945, 0.9296, 1.0); 
				}                    
			}`
	};

	//Mesh
	var planeGeometry = new THREE.PlaneGeometry(60, 20);
	var planeMaterial = new THREE.ShaderMaterial({
		uniforms: customShader.uniforms,
		vertexShader: customShader.vertexShader,
		fragmentShader: customShader.fragmentShader
	});
	var plane = new THREE.Mesh(planeGeometry, planeMaterial);
	scene.add(plane);

	//模型变换  
	plane.position.set(15, 8, -10);
	plane.rotation.x = THREE.MathUtils.DEG2RAD * 30;
	plane.rotation.y = THREE.MathUtils.DEG2RAD * 45;
	plane.rotation.z = THREE.MathUtils.DEG2RAD * 60;

	render();

	var farmeCount = 0;
	function render() {

		var mvpMatrix = new THREE.Matrix4();
		mvpMatrix.multiplyMatrices(camera.projectionMatrix, camera.matrixWorldInverse);
		mvpMatrix.multiplyMatrices(mvpMatrix, plane.matrixWorld);

		customShader.uniforms.mvpMatrix.value = mvpMatrix;
		if (farmeCount % 120 === 0) {
			customShader.uniforms.sw.value = !customShader.uniforms.sw.value;
		}

		renderer.render(scene, camera);
		farmeCount = requestAnimationFrame(render);
	}
})();
