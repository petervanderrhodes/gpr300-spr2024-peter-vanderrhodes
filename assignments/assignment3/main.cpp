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


void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI(ew::Camera* camera, ew::CameraController* cameraController, peter::Framebuffer gBuffer);
void drawShadowUI();
void drawGBufferUI(peter::Framebuffer gBuffer);
void drawScene(ew::Camera camera, ew::Shader shader, ew::Camera lightCamera, bool shouldDrawPlane = true);

//Global state
int screenWidth = 1080;
int screenHeight = 720;
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

ew::Transform planeTransform; // There's probably something about this in the mesh, but I couldn't find it
unsigned int shadowMap;

bool usingPostProcess = false;

glm::vec3 lightDirection = glm::vec3(0, -1, 0);
float cameraDistance = 20;
float angleTest;

float minBias = 0.005;
float maxBias = 0.015;

int main() {
	GLFWwindow* window = initWindow("Assignment 3", screenWidth, screenHeight);
	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader shader2 = ew::Shader("assets/lit.vert", "assets/lit.frag"); // for the plane
	ew::Shader postProcessShader = ew::Shader("assets/postprocess.vert", "assets/postprocess.frag");
	ew::Shader normalShader = ew::Shader("assets/postprocess.vert", "assets/nopostprocess.frag");
	ew::Shader depthShader = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	ew::Shader gBufferShader = ew::Shader("assets/lit.vert", "assets/geometryPass.frag");
	
	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");

	//Plane
	


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

	

	//Create framebuffer
	peter::Framebuffer framebuffer = peter::createFramebuffer(screenWidth, screenHeight, GL_RGB16F);

	//Create G-buffer
	peter::Framebuffer gBuffer = peter::createGBuffer(screenWidth, screenHeight);
	

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer incomplete: %d", fboStatus);
	}

	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

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




	planeTransform.position.y = -2; //Moves plane down

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
		

		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glViewport(0, 0, 2048, 2048);
		glClear(GL_DEPTH_BUFFER_BIT);
		
		//Render scene from light’s point of view
		

		drawScene(lightCamera, depthShader, lightCamera, false);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, shadowMap);

		

		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
		glViewport(0, 0, gBuffer.width, gBuffer.height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawScene(camera, gBufferShader, lightCamera);


		//In render loop...
		//Clears backbuffer color & depth values
		


		//glBindVertexArray(dummyVAO);
		//6 vertices for quad, 3 for triangle
		//glDrawArrays(GL_TRIANGLES, 0, 3);

		//Rotate model around Y axis
		//monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		cameraController.move(window, &camera, deltaTime);

		//Bind brick texture to texture unit 0 
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, brickTexture);

		//or: glBindTextureUnit(0, brickTexture);

		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

		glViewport(0, 0, framebuffer.width, framebuffer.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		


		drawScene(camera, shader, lightCamera);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		if (usingPostProcess) {
			postProcessShader.use();
		}
		else {
			normalShader.use();
		}
		
		

		glBindTextureUnit(0, framebuffer.colorBuffers[0]);
		

		glBindVertexArray(dummyVAO);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		drawUI(&camera, &cameraController, gBuffer);
		


		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void drawUI(ew::Camera* camera, ew::CameraController* cameraController, peter::Framebuffer gBuffer) {
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

	drawShadowUI();

	drawGBufferUI(gBuffer);

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
	shader.setInt("_ShadowMap", 1);

	shader.setFloat("_MinBias", minBias);
	shader.setFloat("_MaxBias", maxBias);

	shader.setFloat("_Material.Ka", material.Ka);
	shader.setFloat("_Material.Kd", material.Kd);
	shader.setFloat("_Material.Ks", material.Ks);
	shader.setFloat("_Material.Shininess", material.Shininess);

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

