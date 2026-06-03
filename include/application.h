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

enum ShapeType
{
	SHAPE_SPHERE = 0,
	SHAPE_BOX = 1,
	SHAPE_PLANE = 2
};

enum OpType
{
	OP_UNION = 0,
	OP_SMOOTH_UNION = 1,
	OP_INTERSECTION = 2,
	OP_SUBTRACTION = 3
};

struct SceneOp
{
	int shapeType;
	int shapeIndex;
	int opType;
	std::string name;
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

	void createShape(int shapeType, int opType);
	void deleteShape(int shapeType, int shapeIndex, int sceneOpIndex);

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
	std::vector<SceneOp> m_sceneOps;

	glm::vec3 m_lightPosition;

	unsigned int m_skyboxTexture;

	bool m_useAA = false;

	int m_maxSteps = 100;

	// Shape defaults
	glm::vec3 sphereCenter = glm::vec3(0.0f);
	float sphereRadius = 1.0f;
	glm::vec3 sphereColor = glm::vec3(1.0f, 0.0f, 0.0f);
	int sphereTex = 0;

	glm::vec3 boxPosition = glm::vec3(0.0f);
	glm::vec3 boxHalfSize = glm::vec3(1.0f);
	float boxRoundRadius = 0.1f;
	glm::vec3 boxColor = glm::vec3(1.0f, 1.0f, 0.0f);
	int boxTex = 0;

	glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
	float planeOffset = 0.0f;
	glm::vec3 planeColor = glm::vec3(0.8f);
	int planeTex = 0;

	void onPressedKey(int key, const std::function<void()>& callback);

	unsigned int loadCubemap(const std::array<std::string, 6>& faces);
	
	void createSphere(glm::vec3 pos);
};
