#pragma once

#include "camera.h"
#include "GLFW/glfw3.h"


class Application
{
public:
	Application();
	~Application();

	void setCallbacks();
	void processInput();
	void deltaTime();

	Camera* camera;
	GLFWwindow* window;
private:
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
