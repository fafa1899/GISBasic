import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import Stats from 'three/examples/jsm/libs/stats.module.js';

import { lakeMaterialGui } from './lib/LakeMaterialGui.js';
import { spatialCoordinateTransform } from './lib/SpatialCoordinateTransform.js'
import LakeLayer from './lib/LakeLayer.js'

'use strict';

//函数立即执行，避免污染全局作用域
(function () {
	const scene = new THREE.Scene();	//根场景
	const layerGroup = new THREE.Group();		//存储GIS数据
	let boundSphere = null;	//包围球

	//湖面要素
	const lakeGroup = new THREE.Group();
	const lakeLayers = [
		new LakeLayer(lakeGroup, "public/lake1.ply", 227.2),
		new LakeLayer(lakeGroup, "public/lake2.ply", 216)
	]

	const stats = new Stats();		// 性能监视器	

	Main();

	//导入DOM作为纹理
	function LoadDomTexture(material) {
		const loader = new THREE.TextureLoader();
		loader.load(
			'public/dom.jpg',
			function (texture) {
				material.uniforms.baseTexture.value = texture;
				material.needsUpdate = true;
			},
			undefined,
			function (err) {
				console.error('An error happened:' + err);
			}
		);
	}


	//创建Mesh
	function CreateTerrainMesh(result) {
		let stringlines = result.split("\r\n");
		if (stringlines.length <= 6) {
			return null;
		}

		let demHeader = [];
		for (let i = 0; i < 6; i++) {
			let subline = stringlines[i].split(/[ ]+/);
			if (subline.length != 2) {
				continue;
			}
			demHeader.push(parseFloat(subline[1]));
		}

		let demWidth = parseInt(demHeader[0]);
		let demHeight = parseInt(demHeader[1]);
		let demStartX = demHeader[2];
		let demStartY = demHeader[3];
		let demDx = demHeader[4];
		let demDy = -demHeader[4];
		demStartX += 0.5 * demDx;
		demStartY += (demHeight - 0.5) * demHeader[4];

		let vertexCount = demHeight * demWidth;
		const positions = new Float32Array(vertexCount * 3);
		const uvs = new Float32Array(vertexCount * 2);
		for (let yi = 0; yi < demHeight; yi++) {
			let subline = stringlines[yi + 6].split(' ');
			for (let xi = 0; xi < demWidth; xi++) {
				let vetextsIndex = demWidth * yi + xi;
				let x = demStartX + xi * demDx;
				let y = demStartY + yi * demDy;
				let z = parseFloat(subline[xi]);

				//UTM坐标转地心直角坐标再转ENU坐标
				const enu = spatialCoordinateTransform.Utm2Enu(new THREE.Vector3(x, y, z));

				positions[vetextsIndex * 3] = enu.x;
				positions[vetextsIndex * 3 + 1] = enu.y;
				positions[vetextsIndex * 3 + 2] = enu.z;
				uvs[vetextsIndex * 2] = parseFloat(xi) / (demWidth - 1);
				uvs[vetextsIndex * 2 + 1] = 1.0 - parseFloat(yi) / (demHeight - 1);
			}
		}

		let girdNum = (demHeight - 1) * (demWidth - 1);
		const indices = new Array(girdNum * 6);
		for (let yi = 0; yi < demHeight - 1; yi++) {
			for (let xi = 0; xi < demWidth - 1; xi++) {
				let indicesIndex = (demWidth - 1) * yi + xi;
				let leftTopVertexIndex = demWidth * yi + xi;
				indices[indicesIndex * 6] = leftTopVertexIndex;
				indices[indicesIndex * 6 + 1] = leftTopVertexIndex + demWidth;
				indices[indicesIndex * 6 + 2] = leftTopVertexIndex + 1;
				indices[indicesIndex * 6 + 3] = leftTopVertexIndex + demWidth;
				indices[indicesIndex * 6 + 4] = leftTopVertexIndex + demWidth + 1;
				indices[indicesIndex * 6 + 5] = leftTopVertexIndex + 1;
			}
		}

		const terrainGeometry = new THREE.BufferGeometry();
		terrainGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
		terrainGeometry.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
		terrainGeometry.setIndex(indices);

		let terrainMaterial = new THREE.ShaderMaterial({
			uniforms: {
				"baseTexture": { value: null }
			},
			vertexShader: `	
			out vec2 textureCoord;	
			void main() { 
				gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 ); 
				textureCoord = uv;
			}
			`,
			fragmentShader: `
			uniform sampler2D baseTexture; 
			in vec2 textureCoord;				      	
			void main() {    
				gl_FragColor = texture(baseTexture, textureCoord); 
			}
			`,
			side: THREE.FrontSide,
		});

		LoadDomTexture(terrainMaterial);

		let terrainMesh = new THREE.Mesh(terrainGeometry, terrainMaterial);
		return terrainMesh;
	}

	//创建地形
	function CreateTerrain() {
		let xhttp = new XMLHttpRequest();
		xhttp.open("GET", "public/dem.asc", true);
		xhttp.onreadystatechange = function () {
			if (this.readyState == 4 && this.status == 200) {
				let terrainMesh = CreateTerrainMesh(this.responseText);
				if (terrainMesh) {
					terrainMesh.geometry.computeBoundingSphere();
					boundSphere = terrainMesh.geometry.boundingSphere;
					layerGroup.add(terrainMesh);
				}
			}
		};
		xhttp.send();
	}

	//设置监视器
	function SetStats() {
		// 设置监视器面板，传入面板id（0: fps, 1: ms, 2: mb）
		stats.setMode(0);
		// 设置监视器位置
		stats.domElement.style.position = 'absolute';
		stats.domElement.style.left = '0px';
		stats.domElement.style.top = '0px';
		// 将监视器添加到页面中
		document.body.appendChild(stats.domElement);

		return stats;
	}


	//加载建筑物
	function LoadBuilding() {
		const loader = new GLTFLoader();
		loader.load('public/LosAngelesBuilding.gltf', function (gltf) {
			layerGroup.add(gltf.scene);
		}, function (xhr) {
		}, function (error) {
			console.error(error);
		});
	}


	// 创建光源
	function CreateLight() {
		//环境光
		const ambientLight = new THREE.AmbientLight(0xffffff, 0.2);
		scene.add(ambientLight);

		//平行光
		const altitude = THREE.MathUtils.degToRad(75);	//太阳高度角
		const azimuth = THREE.MathUtils.degToRad(-45);	//太阳方位角	
		const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
		directionalLight.position.set(Math.cos(altitude) * Math.cos(azimuth), Math.sin(altitude), Math.cos(altitude) * Math.sin(azimuth));
		directionalLight.target.position.set(0, 0, 0);
		scene.add(directionalLight);

		//
		for (let i = 0; i < lakeLayers.length; ++i) {
			lakeLayers[i].LakeMaterial.uniforms.lightSourceDirection.value = directionalLight.position;
		}

		//使用平行光补光	
		const azimuth1 = THREE.MathUtils.degToRad(90);
		const directionalLight1 = new THREE.DirectionalLight(0xffffff, 0.3);
		directionalLight1.position.set(Math.cos(altitude) * Math.cos(azimuth1), Math.sin(altitude), Math.cos(altitude) * Math.sin(azimuth1));
		directionalLight1.target.position.set(0, 0, 0);
		scene.add(directionalLight1);

	}

	// 生成一个canvas对象，标注文字为参数name
	function CreateCanvas(name) {
		const canvas = document.createElement("canvas");

		//计算字符个数
		const charArray = name.split("");
		let charCount = 0;
		const reg = /[\u4e00-\u9fa5]/;
		for (let i = 0; i < charArray.length; i++) {
			if (reg.test(charArray[i])) { //判断是不是汉字
				charCount += 1;
			} else {
				charCount += 0.5; //英文字母或数字累加0.5
			}
		}

		// 计算canvas画布宽高
		const height = 80;
		const width = height + charCount * 32;//设置1个字符32px
		canvas.width = width;
		canvas.height = height;

		//创建2d上下文
		const canvasContext = canvas.getContext('2d');

		//设置填充色
		canvasContext.fillStyle = "rgba(0.00, 0.184, 0.655, 0.5)";

		// 绘制背景框
		const backgroundHeight = height * 0.8;
		const radius = backgroundHeight / 2;
		canvasContext.arc(radius, radius, radius, -Math.PI / 2, Math.PI / 2, true);
		canvasContext.arc(width - radius, radius, radius, Math.PI / 2, -Math.PI / 2, true);
		canvasContext.fill();

		// 绘制箭头	
		canvasContext.beginPath();
		canvasContext.moveTo(width / 2 - (height - backgroundHeight) * 0.6, backgroundHeight);
		canvasContext.lineTo(width / 2 + (height - backgroundHeight) * 0.6, backgroundHeight);
		canvasContext.lineTo(width / 2, height);
		canvasContext.fill();

		// 文字
		canvasContext.beginPath();
		canvasContext.translate(width / 2, backgroundHeight / 2);
		canvasContext.fillStyle = "#ffffff";
		canvasContext.font = "normal 32px 宋体";
		canvasContext.textBaseline = "middle"; //文本与fillText定义的纵坐标
		canvasContext.textAlign = "center"; //文本居中(以fillText定义的横坐标)
		canvasContext.fillText(name, 0, 0);

		return canvas;
	}

	//创建标签
	function CreateLabel(position, name) {
		const canvas = CreateCanvas(name);

		const texture = new THREE.CanvasTexture(canvas);
		texture.minFilter = THREE.LinearFilter;
		texture.wrapS = THREE.ClampToEdgeWrapping;
		texture.wrapT = THREE.ClampToEdgeWrapping;

		const labelMaterial = new THREE.SpriteMaterial({
			map: texture,
			transparent: true,
			depthTest: false,
		});

		const label = new THREE.Sprite(labelMaterial);
		label.position.set(position.x, position.y, position.z);

		const y = 60;//精灵y方向尺寸，宽高比和canvas画布保持一致		
		const x = canvas.width / canvas.height * y;//精灵x方向尺寸
		label.scale.set(x, y, 1);// 控制精灵大小

		layerGroup.add(label);
	}

	//创建标签集
	function CreateLabels(geoJsonData) {
		for (let i = 0; i < geoJsonData.features.length; ++i) {
			let xyz = geoJsonData.features[i].geometry.coordinates;

			const enu = spatialCoordinateTransform.Utm2Enu(new THREE.Vector3(xyz[0], xyz[1], xyz[2] + 30));
			CreateLabel(enu, geoJsonData.features[i].properties.name);
		}
	}

	//导入兴趣点
	function LoadPoi() {
		var xhr = new XMLHttpRequest();
		xhr.open('GET', 'public/poi.geojson', true);
		xhr.onload = function () {
			if (xhr.status === 200) {
				let data = JSON.parse(xhr.responseText);
				CreateLabels(data);
			} else {
				console.error('请求失败：' + xhr.status);
			}
		};
		xhr.onerror = function () {
			console.error('请求失败');
		};
		xhr.send();
	}

	function AddSkyBox() {
		const loader = new THREE.CubeTextureLoader();
		loader.load([
			"public/clouds/left.bmp",
			"public/clouds/right.bmp",
			"public/clouds/top.bmp",
			"public/clouds/bottom.bmp",
			"public/clouds/front.bmp",
			"public/clouds/back.bmp",
		], function (texture) {
			scene.background = texture;
		});
	}

	//创建场景
	function CreateGeoScene() {
		layerGroup.rotateX(THREE.MathUtils.degToRad(-90));
		scene.add(layerGroup);

		CreateTerrain();	//创建地形
		LoadBuilding();		//导入建筑物
		LoadPoi();	//导入兴趣点

		//创建湖面要素
		for (let i = 0; i < lakeLayers.length; ++i) {
			lakeLayers[i].LoadLakeModel();
		}
	}

	//主函数
	function Main() {
		CreateGeoScene();

		CreateLight();	//创建光照
		AddSkyBox(); //增加天空盒

		SetStats();	//设置监视器

		// 相机
		const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 20000);
		camera.position.set(0, 1200, 0);   //相机的位置
		camera.up.set(0, 1, 0);         //相机以哪个方向为上方
		let at = new THREE.Vector3(0, 0, 0);
		camera.lookAt(new THREE.Vector3(0, 0, 0));          //相机看向哪个坐标

		//渲染器
		var renderer = new THREE.WebGLRenderer({
			antialias: true,     //抗锯齿
			physicallyCorrectLights: true,		//物理正确光照			
		});
		renderer.setClearColor(new THREE.Color(0x000000));
		renderer.setPixelRatio(window.devicePixelRatio);//开启HiDPI设置
		renderer.setSize(window.innerWidth, window.innerHeight);
		document.body.appendChild(renderer.domElement);	//add the output of the renderer to the html element

		//控制器
		const clock = new THREE.Clock() //创建THREE.Clock对象，用于计算上次调用经过的时间
		const controls = new OrbitControls(camera, renderer.domElement);

		// 响应窗口大小变化  
		window.addEventListener('resize', function () {
			var width = window.innerWidth;
			var height = window.innerHeight;
			renderer.setSize(width, height);
			camera.aspect = width / height;
			camera.updateProjectionMatrix();
		}, false);


		//渲染纹理	
		const textureRenderTarget = new THREE.WebGLRenderTarget(window.innerWidth, window.innerHeight);//渲染目标缓冲区

		// 监听change事件  
		controls.addEventListener('change', function () {
			if (boundSphere) {
				camera.near = Math.max(camera.position.distanceTo(boundSphere.center) - boundSphere.radius, 10);
				camera.far = camera.position.distanceTo(boundSphere.center) + boundSphere.radius;
				camera.updateProjectionMatrix();
			}

			layerGroup.remove(lakeGroup);
			renderer.setRenderTarget(textureRenderTarget);
			for (let i = 0; i < lakeLayers.length; ++i) {
				renderer.render(scene, lakeLayers[i].ReflectCamera);
				lakeLayers[i].SetReflectData(camera, textureRenderTarget.texture);
			}
			renderer.setRenderTarget(null);
			layerGroup.add(lakeGroup);
		});

		render();

		function render() {
			stats.update();// 更新帧数
			const delta = clock.getDelta(); // 获取自上次调用的时间差
			controls.update(delta); // 控制器更新

			for (let i = 0; i < lakeLayers.length; ++i) {
				lakeLayers[i].LakeMaterial.uniforms.elapsedTime.value = clock.elapsedTime;
			}

			renderer.render(scene, camera);// 渲染界面
			requestAnimationFrame(render);
		}
	}
})();

