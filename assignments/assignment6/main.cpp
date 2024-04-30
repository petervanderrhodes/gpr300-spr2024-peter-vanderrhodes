#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>

#include <peterlib/framebuffer.h>

#include <ew/procGen.h>

#include "assets/bloomRenderer.h"


void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI(ew::Camera* camera, ew::CameraController* cameraController, peter::Framebuffer gBuffer, peter::Framebuffer frameBuffer);
void drawShadowUI();
void drawGBufferUI(peter::Framebuffer gBuffer);
void drawBufferUI(peter::Framebuffer framebuffer);
void drawScene(ew::Camera camera, ew::Shader shader, ew::Camera lightCamera, bool shouldDrawPlane = true);

//Global state
int screenWidth = 1080 * 2;
int screenHeight = 720 * 2;
float prevFrameTime;
float deltaTime;

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

struct PointLight {
	glm::vec3 position;
	float radius;
	glm::vec4 color;
};
const int MAX_POINT_LIGHTS = 64;
PointLight pointLights[MAX_POINT_LIGHTS];


ew::Transform planeTransform; // There's probably something about this in the mesh, but I couldn't find it
unsigned int shadowMap;

bool usingPostProcess = false;
bool usingBloom = true;

glm::vec3 lightDirection = glm::vec3(0, -1, 0);
float cameraDistance = 20;
float angleTest;

float minBias = 0.005;
float maxBias = 0.015;

float bloomRadius = 0.005f;
float exposure = 1.0f;
float bloomStrength = 0.04f;

int main() {
	GLFWwindow* window = initWindow("Assignment 6", screenWidth, screenHeight);
	ew::Shader shader = ew::Shader("assets/fsTriangle.vert", "assets/deferredLit.frag");
	//ew::Shader shader2 = ew::Shader("assets/fsTriangle.vert", "assets/deferredLit.frag"); // for the plane
	ew::Shader postProcessShader = ew::Shader("assets/fsTriangle.vert", "assets/postprocess.frag");
	ew::Shader normalShader = ew::Shader("assets/fsTriangle.vert", "assets/nopostprocess.frag");
	ew::Shader depthShader = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	ew::Shader gBufferShader = ew::Shader("assets/lit.vert", "assets/geometryPass.frag");
	ew::Shader deferredShader = ew::Shader("assets/fsTriangle.vert", "assets/deferredLit.frag");
	ew::Shader lightOrbShader = ew::Shader("assets/lightOrb.vert", "assets/lightOrb.frag");
	ew::Shader bloomShader = ew::Shader("assets/bloomSampling.vert", "assets/bloomSampling.frag");
	

	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");
	

	ew::CameraController cameraController;

	ew::Camera camera;
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	ew::Camera lightCamera;
	lightCamera.target = glm::vec3(0, 0, 0);
	lightCamera.position = lightCamera.target - (lightDirection * cameraDistance);
	lightCamera.orthographic = true;
	lightCamera.orthoHeight = 70.0f;
	lightCamera.aspectRatio = 1;

	//In initialization - create a low res sphere mesh
	ew::Mesh sphereMesh = ew::Mesh(ew::createSphere(1.0f, 8));


	//Create framebuffer
	peter::Framebuffer framebuffer = peter::createFramebuffer(screenWidth, screenHeight, GL_RGB16F);

	//Create G-buffer
	peter::Framebuffer gBuffer = peter::createGBuffer(screenWidth, screenHeight);
	

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	//unsigned int dummyVAO;
	//glCreateVertexArrays(1, &dummyVAO);

	//Create shadowmap
	// TODO? Maybe add this to a file in peterlib
	
	unsigned int shadowFBO;
	glCreateFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	//16 bit depth values, 2k resolution
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, 2048, 2048);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//Pixels outside of frustum should have max distance (white)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	//Tell shadow map to not render color
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);

	//Creating the point lights
	int pointLightDistance = 10;
	int pointLightOffset = 5;
	float pointLightRadius = 8.0f;

	for (int z = 0; z < 8; z++)
	{
		for (int x = 0; x < 8; x++)
		{
			int i = (z * 8) + x;
			pointLights[i].position = glm::vec3((x-4) * pointLightDistance + pointLightOffset, 0, (z-4) * pointLightDistance + pointLightOffset);
			pointLights[i].radius = pointLightRadius;
			float pointLightR = rand() % 256;
			float pointLightG = rand() % 256;
			float pointLightB = rand() % 256;
			glm::vec4 pointLightColor = glm::vec4(pointLightR, pointLightG, pointLightB, 255);
			pointLightColor /= 32;
			pointLights[i].color = pointLightColor;
			
		}
	}


	planeTransform.position.y = -2; //Moves plane down


	//Bloom Renderer
	

	bloomShader.use();
	bloomShader.setInt("scene", 0);
	bloomShader.setInt("bloomBlur", 1);

	BloomRenderer bloomRenderer;
	bloomRenderer.Init(screenWidth, screenHeight);

	//After window initialization...
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		//RENDER

		//UPDATE

		cameraController.move(window, &camera, deltaTime);

		//SHADOW PASS 
		{
			glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
			glViewport(0, 0, 2048, 2048);
			glClear(GL_DEPTH_BUFFER_BIT);

			//Render scene from light’s point of view

			depthShader.setFloat("_MinBias", minBias);
			depthShader.setFloat("_MaxBias", maxBias);
			drawScene(lightCamera, depthShader, lightCamera, false);

			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, shadowMap);
		}






		//Bind brick texture to texture unit 0 
		//GEOMETRY PASS 
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, brickTexture);

			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
			glViewport(0, 0, gBuffer.width, gBuffer.height);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			drawScene(camera, gBufferShader, lightCamera);
		}


		//LIGHTING PASS
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
			
			glViewport(0, 0, framebuffer.width, framebuffer.height);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			lightDirection = glm::normalize(lightDirection);
			lightCamera.position = lightCamera.target - (lightDirection * cameraDistance);
			glm::mat4 lightViewProjection = (lightCamera.projectionMatrix() * lightCamera.viewMatrix());

			deferredShader.use();
			//TODO: SET ALL LIGHTING UNIFORMS

			deferredShader.setVec3("_EyePos", camera.position);

			deferredShader.setMat4("_ViewProjection", camera.projectionMatrix()* camera.viewMatrix());
			deferredShader.setMat4("_LightViewProj", lightViewProjection);

			deferredShader.setFloat("_MinBias", minBias);
			deferredShader.setFloat("_MaxBias", maxBias);

			deferredShader.setFloat("_Material.Ka", material.Ka);
			deferredShader.setFloat("_Material.Kd", material.Kd);
			deferredShader.setFloat("_Material.Ks", material.Ks);
			deferredShader.setFloat("_Material.Shininess", material.Shininess);
			deferredShader.setInt("_ShadowMap",3);

			for (int i = 0; i < MAX_POINT_LIGHTS; i++)
			{
				std::string prefix = "_PointLights[" + std::to_string(i) + "].";
				deferredShader.setVec3(prefix + "position", pointLights[i].position);
				deferredShader.setFloat(prefix + "radius", pointLights[i].radius);
				deferredShader.setVec4(prefix + "color", pointLights[i].color);

			}

			//TODO: BIND GBUFFER TEXTURES
			glBindTextureUnit(0, gBuffer.colorBuffers[0]);
			glBindTextureUnit(1, gBuffer.colorBuffers[1]);
			glBindTextureUnit(2, gBuffer.colorBuffers[2]);

			

			//Shadow Map binding

			glBindTextureUnit(3, shadowMap); 
			// Does the same as:
			//glActiveTexture(GL_TEXTURE3);
			//glBindTexture(GL_TEXTURE_2D, shadowMap);
			
			
			bloomRenderer.configureQuad();
			
			//glBindVertexArray(dummyVAO);
			//glDrawArrays(GL_TRIANGLES, 0, 3);
		}


		//drawScene(camera, shader, lightCamera, true);

		//glClearColor(0.6f, 0.8f, 0.92f, 1.0f);


		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

		
		//RENDER LIGHT ORBS
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.fbo); //Read from gBuffer 
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.fbo); //Write to current fbo
			glBlitFramebuffer(
				0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST
			);

			//Draw all light orbs
			lightOrbShader.use();
			lightOrbShader.setMat4("_ViewProjection", camera.projectionMatrix()* camera.viewMatrix());
			for (int i = 0; i < MAX_POINT_LIGHTS; i++)
			{
				glm::mat4 m = glm::mat4(1.0f);
				m = glm::translate(m, pointLights[i].position);
				m = glm::scale(m, glm::vec3(0.2f)); //Whatever radius you want

				lightOrbShader.setMat4("_Model", m);
				lightOrbShader.setVec3("_Color", pointLights[i].color);
				sphereMesh.draw();
			}

		}




		//drawScene(camera, shader, lightCamera);

		//POST PROCESS PASS 
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			if (usingBloom)
			{
				
				bloomRenderer.RenderBloomTexture(framebuffer.colorBuffers[1], bloomRadius);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				bloomShader.use();
				glBindTextureUnit(0, framebuffer.colorBuffers[0]);
				glBindTextureUnit(1, bloomRenderer.BloomTexture());
				bloomShader.setFloat("exposure", exposure);
				bloomShader.setFloat("bloomStrength", bloomStrength);
				bloomRenderer.configureQuad();
			}
			else {
				if (usingPostProcess) {
					postProcessShader.use();
				}
				else {
					normalShader.use();
				}
				glBindTextureUnit(0, framebuffer.colorBuffers[0]);
				//glBindVertexArray(dummyVAO);
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}



			
			
			
		}
		

		drawUI(&camera, &cameraController, gBuffer, framebuffer);
		


		glfwSwapBuffers(window);
	}
	bloomRenderer.Destroy();
	printf("Shutting down...");
}

void drawUI(ew::Camera* camera, ew::CameraController* cameraController, peter::Framebuffer gBuffer, peter::Framebuffer frameBuffer) {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	ImGui::Text("Add Controls Here!");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(camera, cameraController);
	}

	if (ImGui::CollapsingHeader("Plane Position")) {
		ImGui::SliderFloat("Plane Y", &planeTransform.position.y, -5.0f, 0.0f);
	}

	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}

	if (ImGui::CollapsingHeader("Light Direction")) {
		ImGui::SliderFloat("Light X,", &lightDirection.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Light Y,", &lightDirection.y, -1.0f, 1.0f);
		ImGui::SliderFloat("Light Z,", &lightDirection.z, -1.0f, 1.0f);
		ImGui::SliderAngle("Angle Test", &angleTest, 0.0f, 360.0f); //Was trying to convert the light into angles, but this seems fruitless.
	}

	if (ImGui::CollapsingHeader("Shadow Biases")) {
		ImGui::SliderFloat("Min Bias", &minBias, 0.0f, maxBias);
		ImGui::SliderFloat("Max Bias", &maxBias, minBias, 0.1f);
	}

	if (ImGui::Button("Toggle post process shader")) {
		usingPostProcess = !usingPostProcess;
	}

	if (ImGui::CollapsingHeader("Bloom"))
	{
		if (ImGui::Button("Toggle bloom")) {
			usingBloom = !usingBloom;
		}
		ImGui::SliderFloat("Bloom Radius", &bloomRadius, 0.001f, 0.1f);
		ImGui::SliderFloat("Bloom Strength", &bloomStrength, 0.001f, 1.0f);
	}

	

	//drawShadowUI();

	//drawGBufferUI(gBuffer);
	drawBufferUI(frameBuffer);

	//Add more camera settings here!

	ImGui::End();

	


	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void drawShadowUI()
{
	
	ImGui::Begin("Shadow Map");
	//Using a Child allow to fill all the space of the window.
	ImGui::BeginChild("Shadow Map");
	//Stretch image to be window size
	ImVec2 windowSize = ImGui::GetWindowSize();
	//Invert 0-1 V to flip vertically for ImGui display
	//shadowMap is the texture2D handle
	ImGui::Image((ImTextureID)shadowMap, windowSize, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::EndChild();
	ImGui::End();
	
}

void drawGBufferUI(peter::Framebuffer gBuffer)
{
	ImGui::Begin("GBuffers");
		ImVec2 texSize = ImVec2(gBuffer.width / 4, gBuffer.height / 4);
		for (size_t i = 0; i < 3; i++)
		{
			ImGui::Image((ImTextureID)gBuffer.colorBuffers[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
		}
		ImGui::End();

}

void drawBufferUI(peter::Framebuffer framebuffer)
{
	ImGui::Begin("Frame buffers");
	ImVec2 texSize = ImVec2(framebuffer.width / 4, framebuffer.height / 4);
	for (size_t i = 0; i < 2; i++)
	{
		ImGui::Image((ImTextureID)framebuffer.colorBuffers[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
	}
	ImGui::End();
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

void drawScene(ew::Camera camera, ew::Shader shader, ew::Camera lightCamera, bool shouldDrawPlane)
{
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");

	ew::Transform monkeyTransform;

	lightDirection = glm::normalize(lightDirection);
	lightCamera.position = lightCamera.target - (lightDirection * cameraDistance);
	glm::mat4 lightViewProjection = (lightCamera.projectionMatrix() * lightCamera.viewMatrix());

	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(70, 70, 5));

	shader.use();
	//transform.modelMatrix() combines translation, rotation, and scale into a 4x4 model matrix
	shader.setInt("_MainTex", 0);
	shader.setVec3("_EyePos", camera.position);
	
	shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
	shader.setMat4("_LightViewProj", lightViewProjection);
	//shader.setInt("_ShadowMap", 3);

	

	ew::Transform currentTransform;
	//The monkey array
	
	for (int z = -3; z < 4; z++) {
		for (int x = -3; x < 4; x++) {
			currentTransform.position.x = 10 * x;
			currentTransform.position.z = 10 * z;
			currentTransform.position += monkeyTransform.position;
			shader.setMat4("_Model", currentTransform.modelMatrix());
			monkeyModel.draw(); //Draws monkey model using current shader
		}
	}

	
	

	

	if (shouldDrawPlane)
	{
		shader.setMat4("_Model", planeTransform.modelMatrix());
		//Render plane
		planeMesh.draw();
	}
	
}

