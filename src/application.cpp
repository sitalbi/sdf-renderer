#include "application.h"

#include <array>
#include <iostream>
#include <shader.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>



Application::Application()
{
	m_lastX = 0;
	m_lastY = 0;
	m_firstMouse = true;
}

Application::~Application()
{
	delete(m_camera);
	delete(m_shader);
}

int Application::init()
{
	/* Initialize the library */
	if (!glfwInit())
		return -1;


	/* Create a windowed mode window and its OpenGL context */
	m_window = glfwCreateWindow(window_width, window_height, "SDF", NULL, NULL);
	if (!m_window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(m_window);

	// Load OpenGL functions using glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Hide cursor
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	std::array<float, 12> vertices = {
	-1.0f, -1.0f, 0.0f,
	 1.0f, -1.0f, 0.0f,
	 1.0f,  1.0f, 0.0f,
	-1.0f,  1.0f, 0.0f
	};

	std::array<unsigned int, 6> indices = {
		0, 1, 2,
		0, 2, 3
	};

	// Vertex buffer
	unsigned int vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Vertex array
	glEnableVertexAttribArray(0);

	// Set the position attribute 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	// Index buffer
	unsigned int ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);


	m_camera = new Camera(window_width, window_height, glm::vec3(0, 1, 0), 90.f, 0.f);

	setCallbacks();

	m_spheres.push_back({ glm::vec3(0, 0, 0), 1.f,glm::vec3(1.0, 0, 0) });
	m_spheres.push_back({ glm::vec3(1.5, 0, 0),  0.5f,glm::vec3(0.1, 1, 0.1) });

	m_boxes.push_back({ glm::vec3(0, -2, 0),glm::vec3(2, 1, 1), 0.1f, glm::vec3(1.0, 1.0, 0) });
	m_boxes.push_back({ glm::vec3(0, 2, 0),glm::vec3(1, 0.5, 1), 0.1f, glm::vec3(0.1, 0.1, 1.0) });

	// Create and compile our GLSL program from the shaders
	m_shader = new Shader(RES_DIR "/shaders/default_vert.glsl", RES_DIR"/shaders/sdf_frag.glsl");

	m_shader->bind();

	m_shader->setUniformVec2f("uResolution", m_camera->getResolution());

	m_shader->setUniformVec3f("uLightPosition", glm::vec3(5, 3, 0));
	m_shader->setUniformVec3f("uBackgroundColor", glm::vec3(0.15f));


	

	m_shader->setUniform1i("uSphereCount", m_spheres.size());

	


	m_shader->setUniform1i("uBoxCount", m_boxes.size());

	// Enable depth 
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Enable antialiasing
	glEnable(GL_MULTISAMPLE);

	initUI();

	return 1;
}

void Application::initUI()
{
	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(m_window, true);
	ImGui_ImplOpenGL3_Init("#version 450");
}

void Application::update()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_shader->setUniformMat4f("uInverseViewProj", glm::inverse(m_camera->getProjectionMatrix() * m_camera->getViewMatrix()));
	m_shader->setUniformVec3f("uCameraPos", m_camera->getPosition());

	for (int i = 0; i < m_spheres.size(); i++)
	{
		m_shader->setUniformVec3f("uSpheres[" + std::to_string(i) + "].center", m_spheres[i].center);
		m_shader->setUniform1f("uSpheres["+ std::to_string(i)+"].radius", m_spheres[i].radius);
		m_shader->setUniformVec3f("uSpheres[" + std::to_string(i) + "].color", m_spheres[i].color);
	}

	for (int i = 0; i < m_boxes.size(); i++)
	{
		m_shader->setUniformVec3f("uBoxes[" + std::to_string(i) + "].position", m_boxes[i].position);
		m_shader->setUniformVec3f("uBoxes[" + std::to_string(i) + "].b", m_boxes[i].b);
		m_shader->setUniform1f("uBoxes[" + std::to_string(i) + "].r", m_boxes[i].r);
		m_shader->setUniformVec3f("uBoxes[" + std::to_string(i) + "].color", m_boxes[i].color);
	}

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void Application::updateUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Info Panel 
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiCond_FirstUseEver);
	ImGui::SetWindowCollapsed(false, ImGuiCond_FirstUseEver);
	ImGui::Begin("Info");
	ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(350, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(450, 500), ImGuiCond_FirstUseEver);
	ImGui::SetWindowCollapsed(false, ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene Editor");
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
	int i = 1;
	for (auto& sphere : m_spheres)
	{
		std::string name = "Sphere " + std::to_string(i++);
		if (ImGui::TreeNode(name.c_str()))
		{
			ImGui::DragFloat3("Position", glm::value_ptr(sphere.center), .1f);
			ImGui::DragFloat("Radius", &sphere.radius, 0.1f);
			if (ImGui::TreeNode("Color"))
			{
				ImGui::ColorPicker3("Color", glm::value_ptr(sphere.color));
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}
	i = 1;
	for (auto& box : m_boxes)
	{
		std::string name = "Box " + std::to_string(i++);
		if (ImGui::TreeNode(name.c_str()))
		{
			ImGui::DragFloat3("Position", glm::value_ptr(box.position), .1f);
			ImGui::DragFloat3("Half-Size", glm::value_ptr(box.b), .1f);
			ImGui::DragFloat("Radius", &box.r, 0.1f);
			if (ImGui::TreeNode("Color"))
			{
				ImGui::ColorPicker3("Color", glm::value_ptr(box.color));
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();

}

void Application::run()
{
	while (!glfwWindowShouldClose(m_window))
	{
		deltaTime();

		update();
		updateUI();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_window);
		glfwPollEvents();

		processInput();
	}
	shutdown();
}



void Application::processInput()
{

	if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(m_window, true);
	}


	// TODO: refactor using callbacks
	double xpos, ypos;
	glfwGetCursorPos(m_window, &xpos, &ypos);
	static double lastX = xpos;
	static double lastY = ypos;
	double xoffset = xpos - lastX;
	double yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {

		m_camera->lookRotate(m_deltaTime, xoffset, yoffset);

		// Hide cursor
		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
		// Show cursor
		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}


	// if wasd keys are pressed, move the camera
	if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
		m_camera->moveForward(m_deltaTime);
	}
	if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
		m_camera->moveBackward(m_deltaTime);
	}
	if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
		m_camera->moveLeft(m_deltaTime);
	}
	if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
		m_camera->moveRight(m_deltaTime);
	}
	if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		m_camera->moveUp(m_deltaTime);
	}
	if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		m_camera->moveDown(m_deltaTime);
	}
}

void Application::setCallbacks()
{
	glfwSetWindowUserPointer(m_window, m_camera);

	glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) {
			Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
			cam->scroll(yoffset);
		});
}


void Application::deltaTime()
{
	m_currentFrame = glfwGetTime();
	m_deltaTime = m_currentFrame - m_lastFrame;
	m_lastFrame = m_currentFrame;
}

void Application::shutdown()
{
	glfwTerminate();
}

void Application::onPressedKey(int key, const std::function<void()>& callback)
{
	bool isPressed = glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS;
	if (isPressed && !m_keyStates[key]) {
		callback();
	}
	m_keyStates[key] = isPressed;

}