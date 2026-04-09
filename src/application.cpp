#include "application.h"

Application::Application()
{
	m_lastX = 0;
	m_lastY = 0;
	m_firstMouse = true;
}

Application::~Application()
{
}



void Application::processInput()
{

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}


	// TODO: refactor using callbacks
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	static double lastX = xpos;
	static double lastY = ypos;
	double xoffset = xpos - lastX;
	double yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	camera->lookRotate(m_deltaTime, xoffset, yoffset);

	// if wasd keys are pressed, move the camera
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera->moveForward(m_deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera->moveBackward(m_deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera->moveLeft(m_deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera->moveRight(m_deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		camera->moveUp(m_deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		camera->moveDown(m_deltaTime);
	}
}

void Application::setCallbacks()
{
	glfwSetWindowUserPointer(window, &camera);

	glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
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

void Application::onPressedKey(int key, const std::function<void()>& callback)
{
	bool isPressed = glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS;
	if (isPressed && !m_keyStates[key]) {
		callback();
	}
	m_keyStates[key] = isPressed;

}