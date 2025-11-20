import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';

'use strict';

//函数立即执行，避免污染全局作用域
(function () {
	var scene = new THREE.Scene(); //场景

	Main();

	function Create2DMapMesh() {
		const positions = new Float32Array([
			-180.0, -90.0, 0.0, // v0
			180.0, -90.0, 0.0, // v1
			180.0, 90.0, 0.0, // v2
			-180.0, 90.0, 0.0, // v3
		]);

		const uvs = new Float32Array([
			0.0, 0.0,// v0
			1.0, 0.0, // v1
			1.0, 1.0, // v2
			0.0, 1.0, // v3
		]);

		const indices = [
			0, 1, 2,
			2, 3, 0,
		];

		const planeGeometry = new THREE.BufferGeometry();
		planeGeometry.setIndex(indices);
		planeGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
		planeGeometry.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));

		const loader = new THREE.TextureLoader();	//纹理加载器		
		loader.load(
			'public/BlackMarble_2016_3km.jpg', // resource URL			
			function (texture) {	// onLoad callback				
				const planeMaterial = new THREE.MeshBasicMaterial({
					map: texture
				});
				const plane = new THREE.Mesh(planeGeometry, planeMaterial);
				scene.add(plane);
			},
			undefined,// onProgress callback currently not supported			
			function (err) {// onError callback
				console.error('An error happened.');
			}
		);
	}

	function Main() {
		console.log("Using Three.js version: " + THREE.REVISION);

		//相机
		const gsd = 180 / window.innerHeight;   //分辨率
		const camera = new THREE.OrthographicCamera(-window.innerWidth / 2 * gsd, window.innerWidth / 2 * gsd,
			window.innerHeight / 2 * gsd, -window.innerHeight / 2 * gsd, 0.1, 1000);
		camera.position.set(0, 0, 500);   //相机的位置
		camera.up.set(0, 1, 0);         //相机以哪个方向为上方
		camera.lookAt(new THREE.Vector3(0, 0, 0));          //相机看向哪个坐标

		//渲染器
		var renderer = new THREE.WebGLRenderer({
			antialias: true,     //抗锯齿
		});
		renderer.setClearColor(new THREE.Color(0x000000));
		renderer.setPixelRatio(window.devicePixelRatio);//开启HiDPI设置		
		renderer.setSize(window.innerWidth, window.innerHeight);
		document.body.appendChild(renderer.domElement);	//add the output of the renderer to the html element

		//控制器
		const clock = new THREE.Clock() //创建THREE.Clock对象，用于计算上次调用经过的时间
		const controls = new OrbitControls(camera, renderer.domElement);
		controls.enableRotate = false; // 禁用旋转功能
		controls.update(); // 更新控制器状态

		Create2DMapMesh();

		render();

		function render() {
			const delta = clock.getDelta(); // 获取自上次调用的时间差
			controls.update(delta); // 控制器更新
			renderer.render(scene, camera);
			requestAnimationFrame(render);
		}
	}
})();
