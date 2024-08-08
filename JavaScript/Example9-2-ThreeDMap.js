import * as THREE from 'three';
import { TrackballControls } from 'three/examples/jsm/controls/TrackballControls.js';
import Stats from 'three/examples/jsm/libs/stats.module.js';
import { Blh2Xyz } from './lib/Blh2Xyz.js';

'use strict';

//函数立即执行，避免污染全局作用域
(function () {
	const scene = new THREE.Scene(); //场景
	const stats = new Stats(); // 创建性能监视器

	//创建透视相机
	const camera = new THREE.PerspectiveCamera(60, window.innerWidth / window.innerHeight, 200000, 10000000);

	Main();

	function Create3DMapMesh() {
		const girdNumX = 360;
		const girdNumY = 180;
		const lonStep = 360.0 / girdNumX;
		const latStep = 180.0 / girdNumY;

		const positionBufferSize = (girdNumX + 1) * (girdNumY + 1) * 3;
		const positionBuffer = new Float32Array(positionBufferSize);

		const uvBufferSize = (girdNumX + 1) * (girdNumY + 1) * 2;
		const uvBuffer = new Float32Array(uvBufferSize);

		for (let yi = 0; yi <= girdNumY; ++yi) {
			for (let xi = 0; xi <= girdNumX; ++xi) {

				let x = -180 + lonStep * xi;
				let y = -90 + latStep * yi;
				let geoPoint = new THREE.Vector3(x, y, 0);
				Blh2Xyz(geoPoint);

				let vertexIndex = (girdNumX + 1) * yi + xi;
				positionBuffer[vertexIndex * 3] = geoPoint.y;
				positionBuffer[vertexIndex * 3 + 1] = geoPoint.z;
				positionBuffer[vertexIndex * 3 + 2] = geoPoint.x;
				uvBuffer[vertexIndex * 2] = parseFloat(xi) / girdNumX;
				uvBuffer[vertexIndex * 2 + 1] = parseFloat(yi) / girdNumY;
			}
		}

		const indicesSize = girdNumX * girdNumY * 6;
		const indices = new Array(indicesSize);

		for (let yi = 0; yi < girdNumY; ++yi) {
			for (let xi = 0; xi < girdNumX; ++xi) {
				let indicesIndex = girdNumX * yi * 6 + xi * 6;
				indices[indicesIndex] = (girdNumX + 1) * yi + xi;
				indices[indicesIndex + 1] = (girdNumX + 1) * yi + xi + 1;
				indices[indicesIndex + 2] = (girdNumX + 1) * (yi + 1) + xi + 1;
				indices[indicesIndex + 3] = (girdNumX + 1) * (yi + 1) + xi + 1;
				indices[indicesIndex + 4] = (girdNumX + 1) * (yi + 1) + xi;
				indices[indicesIndex + 5] = (girdNumX + 1) * yi + xi;
			}
		}

		const planeGeometry = new THREE.BufferGeometry();
		planeGeometry.setAttribute('position', new THREE.BufferAttribute(positionBuffer, 3));
		planeGeometry.setAttribute('uv', new THREE.BufferAttribute(uvBuffer, 2));
		planeGeometry.setIndex(indices);

		const loader = new THREE.TextureLoader();	//纹理加载器		
		loader.load(
			'public/BlackMarble_2016_3km.jpg', // resource URL			
			function (texture) {	// onLoad callback				
				const planeMaterial = new THREE.MeshBasicMaterial({
					map: texture
					//side: THREE.DoubleSide, //双面展示
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

	//设置性能监视器
	function SetStats() {
		// 设置监视器面板，传入面板id（0: fps, 1: ms, 2: mb）
		stats.setMode(0);
		// 设置监视器位置
		stats.domElement.style.position = 'absolute';
		stats.domElement.style.left = '0px';
		stats.domElement.style.top = '0px';
		// 将监视器添加到页面中
		document.body.appendChild(stats.domElement);
	}

	//初始化相机
	function InitCamera() {
		let lon = 114;
		let lat = 30;
		let height = 6500000;

		let longitude = lon * THREE.MathUtils.DEG2RAD;
		let latitude = lat * THREE.MathUtils.DEG2RAD;
		let cosLatitude = Math.cos(latitude);

		let front = new THREE.Vector3(cosLatitude * Math.sin(longitude), Math.sin(latitude), cosLatitude * Math.cos(longitude));
		front.normalize();

		let origin = new THREE.Vector3(lon, lat, height);
		Blh2Xyz(origin);
		let right = new THREE.Vector3();
		right.x = origin.x;
		right.y = 0.0;
		right.z = -origin.y;
		right.normalize();

		let up = new THREE.Vector3();
		up.crossVectors(front, right);
		up.normalize();

		camera.position.set(origin.y, origin.z, origin.x);   //相机的位置	
		camera.up.set(up.x, up.y, up.z);         //相机以哪个方向为上方	

		let dst = new THREE.Vector3(lon, lat, 0);
		Blh2Xyz(dst);
		camera.lookAt(dst);  //相机看向哪个坐标

		camera.updateMatrixWorld(true);
	}

	function Main() {
		console.log("Using Three.js version: " + THREE.REVISION);

		//设置性能监视器
		SetStats();

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
		const controls = new TrackballControls(camera, renderer.domElement);

		//相机初始化
		InitCamera();

		Create3DMapMesh();

		render();

		function render() {
			stats.update();// 更新帧数

			const delta = clock.getDelta(); // 获取自上次调用的时间差
			controls.update(delta); // 控制器更新

			renderer.render(scene, camera);
			requestAnimationFrame(render);
		}
	}
})();