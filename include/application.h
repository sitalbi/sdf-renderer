#pragma once

#include <string>

#include "camera.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include <memory>

constexpr int window_width = 1920;
constexpr int window_height = 1080;

class Shader;

struct Material
{
	glm::vec3 albedo = glm::vec3(0.5f,0.5f,0.5f);
	float metallic = 0.5f;
	float roughness = 0.5f;
	float ao = 1.0f;
};

struct Plane
{
	glm::vec3 n;
	float d;
	//glm::vec3 color;
	int texture = 0;
	Material material;
};

struct Sphere
{
	glm::vec3 center;
	float radius;
	//glm::vec3 color;
	int texture = 0;
	Material material;
};

struct Box
{
	glm::vec3 position;
	glm::vec3 b;
	float r;
	//glm::vec3 color;
	int texture = 0;
	Material material;
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
	OP_SUBTRACTION = 3,
	OP_SMOOTH_SUBTRACTION = 4
};

struct SceneOp
{
	int shapeType;
	int shapeIndex;
	int opType;
	std::string name;
};

struct SelectedShape
{
	bool valid = false;
	int opIndex = -1;
};

struct GLEnvironment 
{
	unsigned int hdrTexture = 0;
	unsigned int envCubemap = 0;
	unsigned int irradianceMap = 0;
	unsigned int prefilterMap = 0;
};


class Application
{
public:
	Application();
	~Application();

	int init();
	void initUI();

	void initSkybox();

	void update();
	void updateUI();
	void run();

	void createShape(int shapeType, int opType);
	void eraseShape(int shapeType, int shapeIndex, int sceneOpIndex);

	void setCallbacks();
	void processInput();
	void deltaTime();

	void shutdown();

private:
	GLFWwindow* m_window;
	Camera* m_camera;
	std::unique_ptr<Shader> m_shader;
	std::unique_ptr<Shader> m_equirectangularToCubemapShader;
	std::unique_ptr<Shader> m_irradianceShader;
	std::unique_ptr<Shader> m_prefilterShader;
	std::unique_ptr<Shader> m_brdfShader;

	unsigned int m_fullscreenVAO = 0;
	unsigned int m_fullscreenVBO = 0;
	unsigned int m_fullscreenIBO = 0;

	unsigned int m_captureFBO;
	unsigned int m_captureRBO;

	unsigned int m_brdfLUTTexture;

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
	float m_lightIntensity;
	float m_exposure;

	SelectedShape m_selectedShape;

	//unsigned int m_skyboxTexture;
	GLEnvironment m_environment;

	bool m_useAA = false;

	int m_maxSteps = 100;

	// Shape defaults
	glm::vec3 sphereCenter = glm::vec3(0.0f);
	float sphereRadius = 1.0f;
	int sphereTex = 0;

	glm::vec3 boxPosition = glm::vec3(0.0f);
	glm::vec3 boxHalfSize = glm::vec3(1.0f);
	float boxRoundRadius = 0.1f;
	int boxTex = 0;

	glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
	float planeOffset = 0.0f;
	int planeTex = 0;


	void onPressedKey(int key, const std::function<void()>& callback);

	unsigned int loadCubemap(const std::array<std::string, 6>& faces);
	unsigned int loadHDRImage(const std::string path);

	GLEnvironment buildEnvironmentMaps(unsigned int hdrTexture);


	glm::mat4 getSelectedShapeTransform(const SceneOp& sceneOp) const;
	void applySelectedShapeTransform(const glm::mat4& transform, const SceneOp& sceneOp);
	void updateGizmo();

	void selectShape(int index);

public:

	unsigned int cubeVAO;
	unsigned int cubeVBO;

	void renderCube()
	{
		// initialize (if necessary)
		if (cubeVAO == 0)
		{
			float vertices[] = {
				// back face
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
				 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
				 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
				 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
				-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
				// front face
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
				 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
				 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
				 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
				-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
				// left face
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
				-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
				-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
				// right face
				 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
				 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
				 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
				 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
				 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
				 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
				 // bottom face
				 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
				  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
				  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
				  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
				 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
				 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
				 // top face
				 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
				  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
				  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
				  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
				 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
				 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
			};
			glGenVertexArrays(1, &cubeVAO);
			glGenBuffers(1, &cubeVBO);
			// fill buffer
			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			// link vertex attributes
			glBindVertexArray(cubeVAO);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}
		// render Cube
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}

};
