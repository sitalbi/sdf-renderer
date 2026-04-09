#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class Camera
{
public:

	Camera();
	Camera(int width, int height, glm::vec3 up, float yaw, float pitch);
	~Camera();

	void setFov(float fov);

	glm::mat4 getViewMatrix() const {
		return glm::lookAt(m_position, m_position + m_forward, m_up);

	}
	glm::mat4 getProjectionMatrix() const { return glm::perspective(glm::radians(m_fov), (float)m_width / (float)m_height, m_nearClippingPlane, m_farClippingPlane); }
	glm::vec3 getPosition() const { return m_position; }

	glm::vec3 getForward() const { return m_forward; }
	glm::vec3 getRight() const { return m_right; }
	glm::vec3 getUp() const { return m_up; }

	glm::vec2 getResolution() const { return glm::vec2(m_width, m_height); }

	float getYaw() const { return m_yaw; }
	float getPitch() const { return m_pitch; }
	float getFov() const { return m_fov; }

	void setPosition(glm::vec3 position);

	void updateCameraVectors();

	void lookRotate(float deltaTime, float xoffset, float yoffset);

	void moveForward(float deltaTime);
	void moveBackward(float deltaTime);
	void moveRight(float deltaTime);
	void moveLeft(float deltaTime);
	void moveUp(float deltaTime);
	void moveDown(float deltaTime);

	void scroll(float yoffset);

	void setSpeed(float offset);

	double lastX;
	double lastY;

private:
	unsigned int m_width, m_height;

	float m_sensivity = 10.0f;

	glm::vec3 m_position;
	glm::vec3 m_forward;
	glm::vec3 m_up;
	glm::vec3 m_right;
	glm::vec3 m_worldUp;


	float m_yaw;
	float m_pitch;
	float m_fov = 60.0f;

	float m_farClippingPlane = 300.0;
	float m_nearClippingPlane = 0.1f;

	float m_speed = 3.0f;

};