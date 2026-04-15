#include "Camera.h"

Camera::Camera()
{
}

Camera::Camera(int width, int height, glm::vec3 up, float yaw, float pitch)
{
	m_width = width;
	m_height = height;
	m_worldUp = up;
	m_yaw = yaw;
	m_pitch = pitch;
	m_position = glm::vec3(0.0f, 0.0f, -5.0f);
	updateCameraVectors();
}

Camera::~Camera()
{
}


void Camera::setFov(float fov)
{
	m_fov = fov;
}

void Camera::setPosition(glm::vec3 position)
{
	m_position = position;
	updateCameraVectors();
}

void Camera::updateCameraVectors() {
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_forward = glm::normalize(front);

	m_right = glm::normalize(glm::cross(m_forward, m_worldUp));
	m_up = glm::normalize(glm::cross(m_right, m_forward));
}

void Camera::lookRotate(float deltaTime, float xoffset, float yoffset)
{
	m_yaw += xoffset * m_sensivity * deltaTime;
	m_pitch += yoffset * m_sensivity * deltaTime;

	m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

	updateCameraVectors();
}

void Camera::scroll(float yoffset)
{
	setSpeed(yoffset);
}


void Camera::setSpeed(float offset)
{
	m_speed += offset;
	if (m_speed < 0.1f) m_speed = 0.1f;
	if (m_speed > 100.0f) m_speed = 100.0f;
}

void Camera::setResolution(int width, int height)
{
	m_width = width;
	m_height = height;
}

void Camera::moveForward(float deltaTime)
{
	m_position += m_forward * m_speed * deltaTime;
}

void Camera::moveBackward(float deltaTime)
{
	m_position -= m_forward * m_speed * deltaTime;
}

void Camera::moveRight(float deltaTime)
{
	m_position += m_right * m_speed * deltaTime;
}

void Camera::moveLeft(float deltaTime)
{
	m_position -= m_right * m_speed * deltaTime;
}

void Camera::moveUp(float deltaTime)
{
	m_position += m_up * m_speed * deltaTime;
}

void Camera::moveDown(float deltaTime)
{
	m_position -= m_up * m_speed * deltaTime;
}