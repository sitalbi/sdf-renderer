#pragma once

#include <string>

#include "camera.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"

constexpr int window_width = 1920;
constexpr int window_height = 1080;

class Shader;

struct Plane
{
	glm::vec3 n;
	float d;
	glm::vec3 color;
	int texture = 0;
};

struct Sphere
{
	glm::vec3 center;
	float radius;
	glm::vec3 color;
	int texture = 0;
};

struct Box
{
	glm::vec3 position;
	glm::vec3 b;
	float r;
	glm::vec3 color;
	int texture = 0;
};

class Application
{
public:
	Application();
	~Application();

	int init();
	void initUI();

	void update();
	void updateUI();
	void run();

	void setCallbacks();
	void processInput();
	void deltaTime();

	void shutdown();

private:
	GLFWwindow* m_window;
	Camera* m_camera;
	Shader* m_shader;

	std::unordered_map<int, bool> m_keyStates;

	double m_lastX, m_lastY;
	bool m_firstMouse = true;
	bool m_dockInitialized = false;

	bool m_showEditorUI = true;

	float m_deltaTime = 0.0f;
	float m_lastFrame = 0.0f;
	float m_currentFrame = 0.0f;

	std::vector<Sphere> m_spheres;
	std::vector<Box> m_boxes;
	std::vector<Plane> m_planes;

	glm::vec3 m_lightPosition;

	unsigned int m_skyboxTexture;


	void onPressedKey(int key, const std::function<void()>& callback);

	unsigned int loadCubemap(const std::array<std::string, 6>& faces);
	
	void createSphere(glm::vec3 pos);
};
