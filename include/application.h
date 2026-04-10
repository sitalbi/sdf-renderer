#pragma once

#include "camera.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"

constexpr int window_width = 1920;
constexpr int window_height = 1080;

class Shader;

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

	


	void onPressedKey(int key, const std::function<void()>& callback);
};
