import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import Stats from 'three/examples/jsm/libs/stats.module.js';
import proj4 from 'proj4';

'use strict';

//函数立即执行，避免污染全局作用域
(function () {
	let scene = new THREE.Scene();
	let stats = new Stats();		// 性能监视器

	//
	let utm11Pcs2Wgs84Ecef = {};
	let utm11Pcs2Wgs84Gcs = {};
	let wgs84Gcs2Wgs84Ecef = {};
	let matEcef2Enu = new THREE.Matrix4();//地心坐标转ENU坐标矩阵

	Main();

	//导入DOM作为纹理
	function LoadDomTexture(material) {
		const loader = new THREE.TextureLoader();	
		loader.load(		
			'public/dom.jpg',		
			function (texture) {			
				material.map = texture;
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
			
				let coord = utm11Pcs2Wgs84Ecef.forward([x, y, z]); //UTM坐标转地心直角坐标
				let enu = new THREE.Vector3(coord[0], coord[1], coord[2]);
				enu.applyMatrix4(matEcef2Enu); //地心直角坐标转ENU坐标

				x = enu.x;
				y = enu.y;
				z = enu.z;

				positions[vetextsIndex * 3] = x;
				positions[vetextsIndex * 3 + 1] = y;
				positions[vetextsIndex * 3 + 2] = z;
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

		const terrainMaterial = new THREE.MeshBasicMaterial;
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
					scene.add(terrainMesh);
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
			scene.add(gltf.scene);		
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
		const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
		directionalLight.position.set(0, 0, 1200);
		scene.add(directionalLight);
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

		scene.add(label);
	}

	//创建标签集
	function CreateLabels(geoJsonData) {
		for (let i = 0; i < geoJsonData.features.length; ++i) {
			let xyz = geoJsonData.features[i].geometry.coordinates;
			let coord = utm11Pcs2Wgs84Ecef.forward([xyz[0], xyz[1], xyz[2] + 30]);
			let enu = new THREE.Vector3(coord[0], coord[1], coord[2]);
			enu.applyMatrix4(matEcef2Enu);
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

	//计算Ecef到Enu的转换
	function ComputeEcef2Enu(topocentricOrigin) {
		let radianLon = topocentricOrigin.x * THREE.MathUtils.DEG2RAD;
		let radianLat = topocentricOrigin.y * THREE.MathUtils.DEG2RAD;

		let sinLon = Math.sin(radianLon);
		let cosLon = Math.cos(radianLon);
		let sinLat = Math.sin(radianLat);
		let cosLat = Math.cos(radianLat);

		matEcef2Enu.set(-sinLon, cosLon, 0, 0, -sinLat * cosLon, -sinLat * sinLon,
			cosLat, 0, cosLat * cosLon, cosLat * sinLon, sinLat, 0, 0, 0, 0, 1);

		let coord = wgs84Gcs2Wgs84Ecef.forward([topocentricOrigin.x, topocentricOrigin.y, topocentricOrigin.z]);
		let translatMat = new THREE.Matrix4(1, 0, 0, -coord[0], 0, 1, 0, -coord[1], 0, 0, 1, -coord[2], 0, 0, 0, 1);

		matEcef2Enu.multiply(translatMat);
	}

	//初始化地理参数
	function InitGeoArgument() {		
		let wgs84Gcs = "+proj=longlat +datum=WGS84 +no_defs +type=crs";
		let wgs84Ecef = "+proj=geocent +datum=WGS84 +units=m +no_defs +type=crs";
		let utm11Pcs = "+proj=utm +zone=11 +datum=WGS84 +units=m +no_defs +type=crs";
		utm11Pcs2Wgs84Ecef = proj4(utm11Pcs, wgs84Ecef);
		utm11Pcs2Wgs84Gcs = proj4(utm11Pcs, wgs84Gcs);
		wgs84Gcs2Wgs84Ecef = proj4(wgs84Gcs, wgs84Ecef);

		let centerX = 377967.5;
		let centerY = 3774907.5;
		let centerCoord = utm11Pcs2Wgs84Gcs.forward([centerX, centerY, 0]);
		let topocentricOrigin = new THREE.Vector3(centerCoord[0], centerCoord[1], centerCoord[2]);
		ComputeEcef2Enu(topocentricOrigin);
	}


	//主函数
	function Main() {
		InitGeoArgument();	//初始化地理参数	

		CreateTerrain();	//创建地形
		LoadBuilding();		//导入建筑物
		LoadPoi();	//导入兴趣点

		CreateLight();	//创建光照
		SetStats();	//设置监视器

		// 相机
		const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.1, 20000);
		camera.position.set(0, 0, 1200);   //相机的位置
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

		render();

		function render() {
			stats.update();// 更新帧数
			const delta = clock.getDelta(); // 获取自上次调用的时间差
			controls.update(delta); // 控制器更新

			renderer.render(scene, camera);// 渲染界面
			requestAnimationFrame(render);
		}
	}
})();

