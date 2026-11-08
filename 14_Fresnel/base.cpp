/*
* Laboratorio de Computación Gráfica e Interacción Humano-Computadora
* 14 - Modelo de Fresnel
*/
// AAA
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <sstream>

// GLAD: Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator
// https://glad.dav1d.de/
#include <glad/glad.h>

// GLFW: https://www.glfw.org/
#include <GLFW/glfw3.h>

// GLM: OpenGL Math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// aaa
// Model loading classes
#include <shader_m.h>
#include <camera.h>
#include <model.h>
#include <cubemap.h>
#include <light.h>
#include <material.h>
#include <animatedmodel.h>

#include <irrKlang.h>
using namespace irrklang;

// Functions
bool Start();
bool Update();

// Definición de callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// Gobals
GLFWwindow* window;

// Tamaño en pixeles de la ventana
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Definición de cámara
Camera camera(glm::vec3(0.0f, 2.0f, 10.0f)); 


// Cams
enum CameraMode {
	thirdMode,
	fixedMode
};

CameraMode currentCam = thirdMode;
int currentFixed = 0;

struct FixedCam {
	glm::vec3 position;
	glm::vec3 target;
};

FixedCam fixeds[] = {
	// cam 1 CUADRANTE COMIDA  (+X, -Z)
	// cámara al noroeste del centro (opuesto a +X,-Z), mirando al sureste
	{ glm::vec3(-6.0f, 10.0f,  8.0f),    
		glm::vec3(8.0f,  2.0f, -8.0f) },   

	// cam 2 CUADRANTE KIOSKO  (+X, +Z)
	// cámara al suroeste del centro (opuesto a +X,+Z), mirando al noreste
	{ glm::vec3(-6.0f, 10.0f, -8.0f),
		glm::vec3(8.0f,  2.0f,  8.0f) },

	// cam 3 CUADRANTE LAGO  (-X, +Z)
	// cámara al sureste del centro (opuesto a -X,+Z), mirando al noroeste
	{ glm::vec3(6.0f, 10.0f, -8.0f),
		glm::vec3(-8.0f,  2.0f,  8.0f) },

	// cam 4 CUADRANTE BOSQUE  (-X, -Z)
	// cámara al noreste del centro (opuesto a -X,-Z), mirando al suroeste
	{ glm::vec3(6.0f, 10.0f,  8.0f),
		glm::vec3(-8.0f,  2.0f, -8.0f) }
};

const int NUM_FIXED_CAMERAS = sizeof(fixeds) / sizeof(fixeds[0]);


// Controladores para el movimiento del mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Variables para la velocidad de reproducción
// de la animación
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float elapsedTime = 0.0f;

glm::vec3 charaPosition(0.0f, 0.0f, -30.0f);
glm::vec3 forwardView(0.0f, 0.0f, 1.0f);
float     scaleV = 0.005f;
float     rotateCharacter = 0.0f;
float     rotateTable = 0.0f;


// Shaders
Shader	*cubemapShader;
Shader	*fresnelShader;
Shader	*proceduralShader;
Shader	*dynamicShader;

Shader	*phongShader;

Shader	*waterShader;
Shader	*lakeobjShader;

Shader	*grilloShader;

// Modelos
Model	*base;
Model	*grass;
Model	*gravelPath;
Model	*rockPath;
Model	*flooringRest;

// CUADRANTE LAGO
Model	*agua;
Model	*lilypad;
Model	*lotus;
Model	*bridge;
Model	*bambu;


// DECORACIONES GRAL
Model	*bush;
Model	*monticulos;
Model	*rejas;
Model	*luces;

Model	*fountain;
Model	*flores;
Model	*senializaciones;


// CUADRANTE COMIDA
Model	*puestos;
Model	*mesaspicnic;
Model	*botes;
Model	*sobrePuesto;
Model	*bolsabasura;

// CUADRANTE KIOSKO
Model	*kiosko;
Model	*banio;
Model	*panelsolar;
Model	*bancos;
Model	*banco2;
Model	*macetaplanta;
Model	*bebedor;
Model	*jardineras;
Model	*libreros;
Model	*pararevistas;

// CUADRANTE BOSQUE
Model	*arboles;
Model	*rosas;
Model	*hongos;
Model	*bushbosque;
Model	*rocasmusgo;
Model	*troncos;
Model	*pala;

// Por Keyframes
AnimatedModel	*alien;
AnimatedModel	*mariposa;

// Procedural
Model	*sun;
Model	*moon; // aa
Model	*grillo;

// Cubemap
CubeMap *mainCubeMap;
CubeMap	*nocheCubeMap;
CubeMap	*atardecerCubeMap;

// Materiales
Material material;


// Luces
std::vector<Light> gLights;
Light sunlight;
glm::vec3 sunPos(60.0f, 60.0f, 0.0f);
Light moonlight;
glm::vec3 moonPos(-60.0f, -60.0f, 0.0f);

// Audio
ISoundEngine* SoundEngine = createIrrKlangDevice();

float tradius = 10.0f;
float theta = 0.0f;
float alpha = 0.0f;
float proceduralTime = 0.0f;

// character movement
bool movement = false;




glm::mat4 thirdPOV() {
	// Mapear Zoom (FOV, típicamente 1..45) a distancia de cámara
	// Zoom bajo  -> FOV bajo  -> queremos cerca
	// Zoom alto  -> FOV alto  -> queremos lejos
	const float minRadius = 3.0f;    // no atravesar al personaje
	const float maxRadius = 20.0f;   // no alejarse demasiado
	float radius = glm::mix(minRadius, maxRadius,
		(camera.Zoom - 1.0f) / (45.0f - 1.0f));
	radius = glm::clamp(radius, minRadius, maxRadius);

	// Clamp del pitch para no bajar del suelo
	float pitchDeg = glm::clamp(camera.Pitch, -20.0f, 80.0f);
	float yaw = glm::radians(camera.Yaw);
	float pitch = glm::radians(pitchDeg);

	glm::vec3 offset;
	offset.x = radius * cos(pitch) * sin(yaw);
	offset.y = radius * sin(pitch);
	offset.z = radius * cos(pitch) * cos(yaw);

	glm::vec3 target = charaPosition + glm::vec3(0.0f, 1.5f, 0.0f);
	glm::vec3 cameraPosition = target + offset;

	// Seguro: nunca por debajo del suelo
	const float minHeight = 1.0f;
	if (cameraPosition.y < minHeight) cameraPosition.y = minHeight;

	return glm::lookAt(cameraPosition, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

bool keyPressed(GLFWwindow* window, int key) {
	static std::unordered_map<int, bool> previousState;

	bool currentState = glfwGetKey(window, key) == GLFW_PRESS;
	bool pressed = currentState && !previousState[key];

	previousState[key] = currentState;

	return pressed;
}

void SetLightUniformInt(Shader* shader, const char* propertyName, size_t lightIndex, int value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setInt(uniformName.c_str(), value);
}
void SetLightUniformFloat(Shader* shader, const char* propertyName, size_t lightIndex, float value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setFloat(uniformName.c_str(), value);
}
void SetLightUniformVec4(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec4 value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setVec4(uniformName.c_str(), value);
}
void SetLightUniformVec3(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec3 value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	std::string uniformName = ss.str();

	shader->setVec3(uniformName.c_str(), value);
}

void UploadLights(Shader* shader) {
	shader->setInt("numLights", (int)gLights.size());
	for (size_t i = 0; i < gLights.size(); ++i) {
		SetLightUniformVec3(shader, "Position", i, gLights[i].Position);
		SetLightUniformVec3(shader, "Direction", i, gLights[i].Direction);
		SetLightUniformVec4(shader, "Color", i, gLights[i].Color);
		SetLightUniformVec4(shader, "Power", i, gLights[i].Power);
		SetLightUniformInt(shader, "alphaIndex", i, gLights[i].alphaIndex);
		SetLightUniformFloat(shader, "distance", i, gLights[i].distance);
	}
	shader->setVec3("eye", camera.Position);
}


// -------------------------------------------------- MAIN

// Entrada a función principal
int main()
{
	if (!Start())
		return -1;

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		if (!Update())
			break;
	}

	glfwTerminate();
	return 0;

}

bool Start() {
	// Inicialización de GLFW

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Creación de la ventana con GLFW
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Parque Ecológico y Corredor Ecológico-Urbano", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Ocultar el cursor mientras se rota la escena
	// glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: Cargar todos los apuntadores
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	// Activación de buffer de profundidad
	glEnable(GL_DEPTH_TEST);

	// Compilación y enlace de shaders
	cubemapShader = new Shader("shaders/10_vertex_cubemap.vs", "shaders/10_fragment_cubemap.fs");
	fresnelShader = new Shader("shaders/11_fresnel.vs", "shaders/11_fresnel.fs");

	dynamicShader = new Shader("shaders/09_vertex_skinning.vs", "shaders/09_fragment_skinning.fs");
	grilloShader = new Shader("shaders/grillo.vs", "shaders/grillo.fs");

	phongShader = new Shader("shaders/11_PhongShaderMultLights.vs", "shaders/11_PhongShaderMultLights.fs");
	
	proceduralShader = new Shader("shaders/12_ProceduralAnimation.vs", "shaders/12_ProceduralAnimation.fs");
	waterShader = new Shader("shaders/agua_anim.vs", "shaders/agua_anim.frag");
	lakeobjShader = new Shader("shaders/lakeObjects.vs", "shaders/lakeObjects.fs");


	// Máximo número de huesos: 100 por defecto
	dynamicShader->setBonesIDs(MAX_RIGGING_BONES);

	// Dibujar en malla de alambre
	// glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	// MODELOS
	base = new Model("models/flooring.fbx");
	grass = new Model("models/grass.fbx");
	gravelPath = new Model("models/gravelPath.fbx");
	rockPath = new Model("models/rockPath.fbx");
	flooringRest = new Model("models/flooringRest.fbx");

	agua = new Model("models/agua.fbx");
	lilypad = new Model("models/lilypad.fbx");
	lotus = new Model("models/lotus.fbx");
	bridge = new Model("models/bridge.fbx");
	bambu = new Model("models/bambu.fbx");

	bush = new Model("models/bush.fbx");
	monticulos = new Model("models/monticulos.fbx");
	rejas = new Model("models/rejas.fbx");
	luces = new Model("models/luces.fbx");
	fountain = new Model("models/fountain.fbx");
	flores = new Model("models/flores.fbx");
	senializaciones = new Model("models/senializaciones.fbx");

	puestos = new Model("models/puestos.fbx");
	mesaspicnic = new Model("models/mesaspicnic.fbx");
	botes = new Model("models/botes.fbx");
	sobrePuesto = new Model("models/sobrePuesto.fbx");
	bolsabasura = new Model("models/bolsabasura.fbx");

	kiosko = new Model("models/kiosko.fbx");
	banio = new Model("models/banio.fbx");
	panelsolar = new Model("models/panelsolar.fbx");
	bancos = new Model("models/bancos.fbx");
	banco2 = new Model("models/bancos.fbx");
	macetaplanta = new Model("models/macetaplanta.fbx");
	bebedor = new Model("models/bebedor.fbx");
	jardineras = new Model("models/jardineras.fbx");
	libreros = new Model("models/libreros.fbx");
	pararevistas = new Model("models/pararevistas.fbx");

	arboles = new Model("models/arboles.fbx");
	rosas = new Model("models/rosas.fbx");
	hongos = new Model("models/hongos.fbx");
	bushbosque = new Model("models/bushbosque.fbx");
	rocasmusgo = new Model("models/rocasmusgo.fbx");
	troncos = new Model("models/troncos.fbx");
	pala = new Model("models/pala.fbx");

	// ANIMADOS
	alien = new AnimatedModel("models/alien.fbx");
	mariposa = new AnimatedModel("models/mariposa.fbx");
	sun = new Model("models/sun.fbx");
	moon = new Model("models/moon.fbx");
	grillo = new Model("models/grillo.fbx");

	// Cubemap
	vector<std::string> faces
	{
		"textures/cubemap/01/posx.jpg",
		"textures/cubemap/01/negx.jpg",
		"textures/cubemap/01/posy.jpg",
		"textures/cubemap/01/negy.jpg",
		"textures/cubemap/01/posz.jpg",
		"textures/cubemap/01/negz.jpg"
	};

	mainCubeMap = new CubeMap();
	mainCubeMap->loadCubemap(faces);

	/*vector<std::string> facesNoche
	{
		"textures/cubemap/noche/posx.jpg",
		"textures/cubemap/noche/negx.jpg",
		"textures/cubemap/noche/posy.jpg",
		"textures/cubemap/noche/negy.jpg",
		"textures/cubemap/noche/posz.jpg",
		"textures/cubemap/noche/negz.jpg"
	};

	nocheCubeMap = new CubeMap();
	nocheCubeMap->loadCubemap(facesNoche);


	vector<std::string> facesAtardecer
	{
		"textures/cubemap/atardecer/posx.jpg",
		"textures/cubemap/atardecer/negx.jpg",
		"textures/cubemap/atardecer/posy.jpg",
		"textures/cubemap/atardecer/negy.jpg",
		"textures/cubemap/atardecer/posz.jpg",
		"textures/cubemap/atardecer/negz.jpg"
	};

	atardecerCubeMap = new CubeMap();
	atardecerCubeMap->loadCubemap(facesAtardecer);*/


	Light light01;
	light01.Position = glm::vec3(0.0f, 0.5f, 0.0f);
	light01.Color = glm::vec4(0.98f, 0.9411f, 0.7921f, 1.0f);
	light01.Power = glm::vec4(20.0f, 20.0f, 20.0f, 1.0f);
	gLights.push_back(light01);

	Light light02;
	light02.Position = glm::vec3(1.0f, 0.5f, 0.0f);
	light02.Color = glm::vec4(0.98f, 0.9411f, 0.7921f, 1.0f);
	light02.Power = glm::vec4(20.0f, 20.0f, 20.0f, 1.0f);
	gLights.push_back(light02);

	sunlight.Position = sunPos;
	sunlight.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sunlight.Color = glm::vec4(0.9568f, 0.8274f, 0.3686f, 1.0f); // #f4d35e
	sunlight.Power = glm::vec4(200.0f, 200.0f, 200.0f, 1.0f);
	sunlight.alphaIndex = 80;
	sunlight.distance = 35.0f;
	gLights.push_back(sunlight);

	moonlight.Position = moonPos;
	moonlight.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	moonlight.Color = glm::vec4(0.8784f, 0.8823f, 0.8666f, 1.0f); // #e0e1dd
	moonlight.Power = glm::vec4(150.0f, 150.0f, 150.0f, 1.0f);
	moonlight.alphaIndex = 80;
	moonlight.distance = 35.0f;
	gLights.push_back(moonlight);

	material.ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	material.diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	material.specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	material.transparency = 1.0f;

	SoundEngine->play2D("sound/Nature.mp3", true);

	return true;
}

bool Update() {
	// Cálculo del framerate
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Procesa la entrada del teclado o mouse
	processInput(window);

	// Renderizado R - G - B - A
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
	// glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 view;
	if (currentCam == thirdMode) {
		view = thirdPOV();
	}
	else {
		view = glm::lookAt(fixeds[currentFixed].position, fixeds[currentFixed].target, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	/* // En Update(), antes de dibujar el suelo/parque:
	std::vector<glm::vec3> cercanas = farolasPos;
	const int N = 8;   // deja sitio para sol, luna y 2 luces fijas
	std::partial_sort(cercanas.begin(), cercanas.begin() + N, cercanas.end(),
		[&](const glm::vec3& a, const glm::vec3& b) {
			return glm::distance(a, charaPosition) < glm::distance(b, charaPosition);
		});

	// Reconstruye gLights dinámicamente (o actualiza solo las farolas)
	gLights.clear();
	gLights.push_back(sunlight);
	gLights.push_back(moonlight);
	for (int i = 0; i < N; i++) {
		Light l;
		l.Position = cercanas[i];
		l.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
		l.Color = glm::vec4(1.0f, 0.75f, 0.40f, 1.0f);
		l.Power = glm::vec4(40.0f, 40.0f, 40.0f, 1.0f);
		l.alphaIndex = 16;
		l.distance = 8.0f;
		gLights.push_back(l);
	} */

	// Cubemap (fondo)
	{
		mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
	}

	//  SUELOOO
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		base->Draw(*phongShader);
	}

	glUseProgram(0);

	//  pasto
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		grass->Draw(*phongShader);
	}

	glUseProgram(0);


	//  GRAVA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		gravelPath->Draw(*phongShader);
	}

	glUseProgram(0);

	//  SENDERO ROCA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		rockPath->Draw(*phongShader);
	}

	glUseProgram(0);

	// SUELO DESCANSO
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		flooringRest->Draw(*phongShader);
	}

	glUseProgram(0);

	// LAGUITO
	{
		// Activamos el shader del plano
		waterShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		waterShader->setMat4("projection", projection);
		waterShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		waterShader->setMat4("model", model);

		waterShader->setFloat("time", (float)glfwGetTime());

		agua->Draw(*waterShader);

		glDisable(GL_BLEND);
	}

	glUseProgram(0);

	// NENUFARES
	{
		// Activamos el shader del plano
		lakeobjShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		lakeobjShader->setMat4("projection", projection);
		lakeobjShader->setMat4("view", view);

		lakeobjShader->setFloat("lateralAmplitude", -0.65f);
		lakeobjShader->setFloat("lateralSpeed", 0.8f);
		lakeobjShader->setVec3("lateralDir", glm::vec3(1.0f, 0.0f, 0.0f));
		lakeobjShader->setFloat("swayAmount", 0.02f);
		lakeobjShader->setFloat("swaySpeed", 1.5f);
		lakeobjShader->setFloat("time", (float)glfwGetTime());

		// Aplicamos transformaciones del modelo
		glm::vec3 groupPos(0.0f, 1.0f, 0.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, groupPos);
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
		model = glm::scale(model, glm::vec3(0.4f));
		lakeobjShader->setMat4("model", model);

		// Un solo centro para el grupo
		lakeobjShader->setVec3("objectCenter", groupPos);

		lakeobjShader->setVec3("viewPos", camera.Position);
		UploadLights(lakeobjShader);

		lakeobjShader->setVec4("MaterialAmbientColor", material.ambient);
		lakeobjShader->setVec4("MaterialDiffuseColor", material.diffuse);
		lakeobjShader->setVec4("MaterialSpecularColor", material.specular);
		lakeobjShader->setFloat("transparency", material.transparency);

		lilypad->Draw(*lakeobjShader);

		glDisable(GL_BLEND);
	}

	glUseProgram(0);

	// FLOR DE LOTO
	{
		// Activamos el shader del plano
		lakeobjShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		lakeobjShader->setMat4("projection", projection);
		lakeobjShader->setMat4("view", view);

		lakeobjShader->setFloat("lateralAmplitude", -0.65f);
		lakeobjShader->setFloat("lateralSpeed", 0.8f);
		lakeobjShader->setVec3("lateralDir", glm::vec3(1.0f, 0.0f, 0.0f));
		lakeobjShader->setFloat("swayAmount", 0.02f);
		lakeobjShader->setFloat("swaySpeed", 1.5f);
		lakeobjShader->setFloat("time", (float)glfwGetTime());

		// Aplicamos transformaciones del modelo
		glm::vec3 groupPos(0.0f, 1.0f, 0.0f);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, groupPos);
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
		model = glm::scale(model, glm::vec3(0.4f));
		lakeobjShader->setMat4("model", model);

		// Un solo centro para el grupo
		lakeobjShader->setVec3("objectCenter", groupPos);

		lakeobjShader->setVec3("viewPos", camera.Position);
		UploadLights(lakeobjShader);

		lakeobjShader->setVec4("MaterialAmbientColor", material.ambient);
		lakeobjShader->setVec4("MaterialDiffuseColor", material.diffuse);
		lakeobjShader->setVec4("MaterialSpecularColor", material.specular);
		lakeobjShader->setFloat("transparency", material.transparency);

		lotus->Draw(*lakeobjShader);

		glDisable(GL_BLEND);
	}

	glUseProgram(0);

	// SUELO DESCANSO
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		bridge->Draw(*phongShader);
	}

	glUseProgram(0);

	// BAMBU
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		bambu->Draw(*phongShader);
	}

	glUseProgram(0);

	// ARBUSTOS
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		bush->Draw(*phongShader);
	}

	glUseProgram(0);

	// MONTICULOS
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		monticulos->Draw(*phongShader);
	}

	glUseProgram(0);

	// REJA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		rejas->Draw(*phongShader);
	}

	glUseProgram(0);

	// LUCES
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		luces->Draw(*phongShader);
	}

	glUseProgram(0);


	//  FUENTE MARBLE
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		fountain->Draw(*phongShader);
	}

	glUseProgram(0);

	//  FLORES
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.3f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		flores->Draw(*phongShader);
	}

	glUseProgram(0);

	//  FUENTE MARBLE
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		senializaciones->Draw(*phongShader);
	}

	glUseProgram(0);

	//  PUESTOS COMIDA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		puestos->Draw(*phongShader);
	}

	glUseProgram(0);

	//  PUESTOS COMIDA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		sobrePuesto->Draw(*phongShader);
	}

	glUseProgram(0);

	//  MESAS COMIDA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.8f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		mesaspicnic->Draw(*phongShader);
	}

	glUseProgram(0);

	//  BOTES BASURA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(44.0f, -0.6f, -15.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		botes->Draw(*phongShader);
	}

	glUseProgram(0);

	// BOLSA BASURA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		bolsabasura->Draw(*phongShader);
	}

	glUseProgram(0);

	// KIOSKO
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		kiosko->Draw(*phongShader);
	}

	glUseProgram(0);

	//  BAÑO
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		banio->Draw(*phongShader);
	}

	glUseProgram(0);

	//  panelSolar
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		panelsolar->Draw(*phongShader);
	}

	glUseProgram(0);

	//  BANCOS
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(30.0f, -1.0f, -48.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.04f, 0.04f, 0.04f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		bancos->Draw(*phongShader);
	}

	glUseProgram(0);

	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(20.0f, -1.0f, 18.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(215.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.04f, 0.04f, 0.04f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		banco2->Draw(*phongShader);
	}

	glUseProgram(0);

	// MACETAS
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		macetaplanta->Draw(*phongShader);
	}

	glUseProgram(0);

	// BEBEDOR
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		bebedor->Draw(*phongShader);
	}

	glUseProgram(0);

	// JARDINERAS
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(22.0f, -1.3f, 16.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		jardineras->Draw(*phongShader);
	}

	glUseProgram(0);

	// LIBREROS
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(11.0f, -1.0f, 39.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		libreros->Draw(*phongShader);
	}

	glUseProgram(0);

	// REVISTERO
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(42.0f, -0.4f, 50.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		pararevistas->Draw(*phongShader);
	}

	glUseProgram(0);


	// ARBOLES
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		arboles->Draw(*phongShader);
	}

	glUseProgram(0);

	
	// ROSAS
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		rosas->Draw(*phongShader);
	}

	glUseProgram(0);

	// HONGOS
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		hongos->Draw(*phongShader);
	}

	glUseProgram(0);

	// bushBOSQUE
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		bushbosque->Draw(*phongShader);
	}

	glUseProgram(0);

	// ROCAS MUSGO
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		rocasmusgo->Draw(*phongShader);
	}

	glUseProgram(0);

	// troncos
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-30.0f, -1.0f, -10.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		troncos->Draw(*phongShader);
	}

	glUseProgram(0);

	// PALA
	{
		// Activamos el shader del plano
		phongShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		phongShader->setMat4("projection", projection);
		phongShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));	// it's a bit too big for our scene, so scale it down
		phongShader->setMat4("model", model);

		// Configuramos propiedades de fuentes de luz
		UploadLights(phongShader);

		phongShader->setVec3("eye", camera.Position);

		phongShader->setVec4("MaterialAmbientColor", material.ambient);
		phongShader->setVec4("MaterialDiffuseColor", material.diffuse);
		phongShader->setVec4("MaterialSpecularColor", material.specular);
		phongShader->setFloat("transparency", material.transparency);

		pala->Draw(*phongShader);
	}

	glUseProgram(0);


	// ANIMADOS ----------------------------------------------------------------------------------------------------------------------



	{
		// Dibujar personaje
		{
			// Actualización de la animación
			if (movement) {
				alien->UpdateAnimation(deltaTime);

			}
			// Activación del shader del personaje
			dynamicShader->use();

			// Aplicamos transformaciones de proyección y cámara (si las hubiera)
			dynamicShader->setMat4("projection", projection);
			dynamicShader->setMat4("view", view);

			// Aplicamos transformaciones del modelo
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, charaPosition); // translate it down so it's at the center of the scene
			model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));	// it's a bit too big for our scene, so scale it down

			dynamicShader->setMat4("model", model);

			dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, alien->gBones);

			UploadLights(dynamicShader);

			dynamicShader->setVec4("MaterialAmbientColor", material.ambient);
			dynamicShader->setVec4("MaterialDiffuseColor", material.diffuse);
			dynamicShader->setVec4("MaterialSpecularColor", material.specular);
			dynamicShader->setFloat("transparency", material.transparency);

			// Dibujamos el modelo
			alien->Draw(*dynamicShader);
		}

	}

	glUseProgram(0);

	{
		// Dibujar personaje
		{
			// Actualización de la animación
			mariposa->UpdateAnimation(deltaTime);
			// Activación del shader del personaje
			dynamicShader->use();

			// Aplicamos transformaciones de proyección y cámara (si las hubiera)
			dynamicShader->setMat4("projection", projection);
			dynamicShader->setMat4("view", view);

			// Aplicamos transformaciones del modelo
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, charaPosition); // translate it down so it's at the center of the scene
			model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(0.008f, 0.008f, 0.008f));	// it's a bit too big for our scene, so scale it down

			dynamicShader->setMat4("model", model);

			dynamicShader->setMat4("gBones", MAX_RIGGING_BONES, mariposa->gBones);

			UploadLights(dynamicShader);

			dynamicShader->setVec4("MaterialAmbientColor", material.ambient);
			dynamicShader->setVec4("MaterialDiffuseColor", material.diffuse);
			dynamicShader->setVec4("MaterialSpecularColor", material.specular);
			dynamicShader->setFloat("transparency", material.transparency);

			// Dibujamos el modelo
			mariposa->Draw(*dynamicShader);
		}

	}

	glUseProgram(0);


	// --- GRILLO PROCEDURAL ---
	{
		grilloShader->use();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		grilloShader->setMat4("projection", projection);
		grilloShader->setMat4("view", view);

		// Posición base del grillo en el mundo
		glm::vec3 basePos(-20.0f, 0.0f, 3.0f);   // ajústalo a tu escena

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, basePos);
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));  // Blender Z-up
		model = glm::scale(model, glm::vec3(0.4f));
		grilloShader->setMat4("model", model);

		// Parámetros del salto
		grilloShader->setFloat("time", (float)glfwGetTime());
		grilloShader->setFloat("jumpHeight", 1.2f);
		grilloShader->setFloat("jumpDistance", 2.0f);
		grilloShader->setFloat("jumpDuration", 0.7f);
		grilloShader->setFloat("pauseDuration", 0.5f);

		// Patas
		grilloShader->setFloat("legSwing", 0.6f);
		grilloShader->setFloat("legSwingSpeed", 8.0f);

		// Posición base (para fase/desfase si tienes varios grillos)
		grilloShader->setVec3("basePosition", basePos);

		// Luces (igual que el resto de modelos)
		grilloShader->setVec3("viewPos", camera.Position);
		grilloShader->setInt("numLights", (int)gLights.size());
		for (size_t i = 0; i < gLights.size(); ++i) {
			SetLightUniformVec3(grilloShader, "Position", i, gLights[i].Position);
			SetLightUniformVec3(grilloShader, "Direction", i, gLights[i].Direction);
			SetLightUniformVec4(grilloShader, "Color", i, gLights[i].Color);
			SetLightUniformVec4(grilloShader, "Power", i, gLights[i].Power);
			SetLightUniformInt(grilloShader, "alphaIndex", i, gLights[i].alphaIndex);
			SetLightUniformFloat(grilloShader, "distance", i, gLights[i].distance);
		}
		grilloShader->setVec4("MaterialAmbientColor", material.ambient);
		grilloShader->setVec4("MaterialDiffuseColor", material.diffuse);
		grilloShader->setVec4("MaterialSpecularColor", material.specular);
		grilloShader->setFloat("transparency", material.transparency);

		grillo->Draw(*grilloShader);

		glDisable(GL_BLEND);
	}

	glUseProgram(0);

	// SOOOOOOOOOOOOOL
	{
		// Activamos el shader de Phong
		proceduralShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Aplicamos transformaciones de proyección y cámara (si las hubiera)
		proceduralShader->setMat4("projection", projection);
		proceduralShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
		proceduralShader->setMat4("model", model);

		proceduralShader->setFloat("time", proceduralTime);
		proceduralShader->setFloat("radius", 40.0f);
		proceduralShader->setFloat("height", 0.5f);

		sun->Draw(*proceduralShader);
		proceduralTime += 0.001;

	}

	glUseProgram(0);

	// LUNAAAAAAAAAAAAAAAAAAAAA
	{
		// Activamos el shader de Phong
		proceduralShader->use();

		// Activamos para objetos transparentes
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Aplicamos transformaciones de proyección y cámara (si las hubiera)
		proceduralShader->setMat4("projection", projection);
		proceduralShader->setMat4("view", view);

		// Aplicamos transformaciones del modelo
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
		proceduralShader->setMat4("model", model);

		proceduralShader->setFloat("time", proceduralTime + glm::pi<float>());
		proceduralShader->setFloat("radius", 30.0f);
		proceduralShader->setFloat("height", 0.0f);

		moon->Draw(*proceduralShader);
		proceduralTime += 0.001;

	}

	glUseProgram(0);

	// glfw: swap buffers 
	glfwSwapBuffers(window);
	glfwPollEvents();

	return true;
}

// Procesamos entradas del teclado
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		deltaTime *= 3.5f;
	/*
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
		*/
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	if (keyPressed(window, GLFW_KEY_T)) {
		currentCam = thirdMode;
	}

	if (keyPressed(window, GLFW_KEY_1)) { //CAM1
		currentCam = fixedMode;
		currentFixed = 0;
	}

	if (keyPressed(window, GLFW_KEY_2)) { //CAM2
		currentCam = fixedMode;
		currentFixed = 1;
	}
	if (keyPressed(window, GLFW_KEY_3)) { //CAM3
		currentCam = fixedMode;
		currentFixed = 2;
	}
	if (keyPressed(window, GLFW_KEY_4)) { //CAM3
		currentCam = fixedMode;
		currentFixed = 3;
	}

	// character movement
	float movementSpeed = 2.0f * deltaTime;
	float rotationSpeed = 30.0f * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		//charaPosition = charaPosition + scaleV * forwardView;
		charaPosition += forwardView * movementSpeed;
		movement = true;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		// charaPosition = charaPosition - scaleV * forwardView;
		charaPosition -= forwardView * movementSpeed;
		movement = true;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		rotateCharacter += rotationSpeed;
		movement = true;
		/*rotateCharacter += 0.5f;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		forwardView = glm::vec3(viewVector);
		forwardView = glm::normalize(forwardView);*/
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		rotateCharacter -= rotationSpeed;
		movement = true;
		/*rotateCharacter -= 0.5f;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec4 viewVector = model * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		forwardView = glm::vec3(viewVector);
		forwardView = glm::normalize(forwardView);*/
	}

	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotateCharacter), glm::vec3(0.0f, 1.0f, 0.0f));

	forwardView = glm::normalize(glm::vec3(rotation * glm::vec4(0, 0, 1.5, 0)));
	
}

// glfw: Actualizamos el puerto de vista si hay cambios del tamaño
// de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// glfw: Callback del movimiento y eventos del mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos; 

	lastX = (float)xpos;
	lastY = (float)ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: Complemento para el movimiento y eventos del mouse
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll((float)yoffset);
}