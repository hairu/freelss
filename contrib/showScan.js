	var scene = new THREE.Scene();
	var camera = new THREE.PerspectiveCamera( 30, window.innerWidth/window.innerHeight, 1.0, 10000 );
	var controls;
	var PI2 = Math.PI * 2;	
	var canvasProgram = function ( context ) {
		//context.beginPath();
		//context.arc( 0, 0, 0.5, 0, PI2, true );		
		//context.fill();
		context.beginPath();
		context.fillRect(0, 0, 1.0, 1.0);
		context.fill();
	};

	function webglAvailable() {
		try {
			var canvas = document.createElement( 'canvas' );
			return !!( window.WebGLRenderingContext && (
				canvas.getContext( 'webgl' ) ||
				canvas.getContext( 'experimental-webgl' ) )
			);
		} catch ( e ) {
			return false;
		}
	}

	function addPointsWebGL(event) {
		var geometry = event.content;
		var pcMaterial = new THREE.PointsMaterial( {
			size: 1.75,
			vertexColors: THREE.VertexColors
		} );
		geometry.useColor = true;
		var mesh = new THREE.Points( geometry, pcMaterial );
		scene.add( mesh );
	}
	
	function addPointsCanvas(event) {
		var geometry = event.content;
		var vertices = geometry.vertices;
		var colors = geometry.colors;
		var group = new THREE.Group();
		scene.add( group );
		for ( var i = 0; i < vertices.length; i+=canvasVertexSkip ) {
			vertex = vertices[i];
			color = colors[i];
			var material = new THREE.SpriteCanvasMaterial( {
				color: color,
				program: canvasProgram
			} );
			particle = new THREE.Sprite( material );			
			particle.position.x = vertex.x;
			particle.position.y = vertex.y;
			particle.position.z = vertex.z;
			particle.scale.x = particle.scale.y = 1;
			group.add( particle );
		}
	}
	
	if (useWebGL) {
		useWebGL = webglAvailable();
	}
	
	if (useWebGL) {
		renderer = new THREE.WebGLRenderer({ alpha: true });
	} else {
		renderer = new THREE.CanvasRenderer();
		renderer.setPixelRatio( window.devicePixelRatio );
	}	
	
	renderer.setSize( window.innerWidth, window.innerHeight );
	renderer.setClearColor( 0x555555, 1);

	document.body.appendChild( renderer.domElement );
	window.addEventListener( 'resize', onWindowResize, false );

	var loader = new THREE.PLYLoader();
	if (useWebGL) {	
		loader.addEventListener( 'load', addPointsWebGL);
	} else {
		loader.addEventListener( 'load', addPointsCanvas);
	}
	
	loader.load(plyFilename);

	var cylHeight = 1;
	var cylRadius = 6 * 25.4;
	var cylGeometry = new THREE.CylinderGeometry( cylRadius, cylRadius, cylHeight, 64 );
	var cylMaterial = new THREE.MeshBasicMaterial( {color: 0x222222 } );
	var cylinder = new THREE.Mesh( cylGeometry, cylMaterial );
	cylinder.translateY( -cylHeight/2 );
	scene.add( cylinder );

	var cylHeight2 = 8;
	var cylRadius2 = 6 * 25.4;
	var cylGeometry2 = new THREE.CylinderGeometry( cylRadius2, cylRadius2, cylHeight2, 64 );
	var cylMaterial2 = new THREE.MeshBasicMaterial( { color: 0xaaaaaa } );
	var cylinder2 = new THREE.Mesh( cylGeometry2, cylMaterial2 );
	cylinder2.translateY( -cylHeight2/2 - cylHeight );
	scene.add( cylinder2 );

	camera.position.z = 350;
	camera.position.y = 450;
	controls = new THREE.OrbitControls( camera );
	controls.maxDistance = 1000;

	var render = function () {
		requestAnimationFrame( render );
		renderer.render(scene, camera);
	};

	render();

	
	function onWindowResize() {
		windowHalfX = window.innerWidth / 2;
		windowHalfY = window.innerHeight / 2;
		camera.aspect = window.innerWidth / window.innerHeight;
		camera.updateProjectionMatrix();
		renderer.setSize( window.innerWidth, window.innerHeight );
	}
