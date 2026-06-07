#include "application.h"

#include <array>
#include <iostream>
#include <shader.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ImGuizmo.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



Application::Application()
{
	m_lastX = 0;
	m_lastY = 0;
	m_firstMouse = true;
}

Application::~Application()
{
	if (m_fullscreenIBO != 0) glDeleteBuffers(1, &m_fullscreenIBO);

	if (m_fullscreenVBO != 0) glDeleteBuffers(1, &m_fullscreenVBO);

	if (m_fullscreenVAO != 0) glDeleteVertexArrays(1, &m_fullscreenVAO);

	delete(m_camera);
}

int Application::init()
{
	// Initialize the library 
	if (!glfwInit())
		return -1;


	// Create a windowed mode window and its OpenGL context
	m_window = glfwCreateWindow(window_width, window_height, "SDF", NULL, NULL);
	if (!m_window)
	{
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
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

	cubeVAO = 0;
	cubeVBO = 0;

	// Vertex array
	glGenVertexArrays(1, &m_fullscreenVAO);
	glBindVertexArray(m_fullscreenVAO);

	// Vertex buffer
	glGenBuffers(1, &m_fullscreenVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Index buffer
	glGenBuffers(1, &m_fullscreenIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_fullscreenIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	// Vertex array attributes
	glEnableVertexAttribArray(0); 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	m_camera = new Camera(window_width, window_height, glm::vec3(0, 1, 0), 90.f, 0.f);

	std::array<std::string, 6> faces =
	{
		RES_DIR"/textures/skybox/skybox_rt.png",
		RES_DIR"/textures/skybox/skybox_lf.png",
		RES_DIR"/textures/skybox/skybox_up.png",
		RES_DIR"/textures/skybox/skybox_dn.png",
		RES_DIR"/textures/skybox/skybox_bk.png",
		RES_DIR"/textures/skybox/skybox_ft.png",
	};


	setCallbacks();

	m_spheres.push_back({ glm::vec3(0, 0, 0), 1.f});
	m_spheres.push_back({ glm::vec3(1.5, 0, 0),  0.5f});

	m_boxes.push_back({ glm::vec3(0, -2, 0),glm::vec3(2, 1, 1), 0.1f });
	m_boxes.push_back({ glm::vec3(0, 2, 0),glm::vec3(1, 0.5, 1), 0.1f });
	m_boxes.push_back({ glm::vec3(0, -4, 0),glm::vec3(20, 0.1, 20), 0.1f, 1 });

	m_sceneOps.clear();
	m_sceneOps.push_back({ SHAPE_SPHERE, 0, OP_UNION});
	m_sceneOps.push_back({ SHAPE_SPHERE, 1, OP_SMOOTH_UNION});
	m_sceneOps.push_back({ SHAPE_BOX,    0, OP_SMOOTH_UNION });
	m_sceneOps.push_back({ SHAPE_BOX,    1, OP_SMOOTH_UNION });
	m_sceneOps.push_back({ SHAPE_BOX,    2, OP_SMOOTH_UNION });

	m_lightPosition = glm::vec3(10, 5, 0);
	m_lightIntensity = 1.0f;
	m_exposure = 1.0f;

	// Create and compile our GLSL programs from the raymarching shaders
	m_shader = std::make_unique<Shader>(RES_DIR "/shaders/default_vert.glsl", RES_DIR"/shaders/sdf_frag.glsl");

	initSkybox();

	//m_skyboxTexture = loadCubemap(faces);
	unsigned int hdrTexture = loadHDRImage(RES_DIR"/textures/skybox/autumn_field_puresky_4k.hdr");
	m_environment = buildEnvironmentMaps(hdrTexture);

	m_shader->bind();
	m_shader->setUniformVec2f("uResolution", m_camera->getResolution());
	m_shader->setUniform1i("uSkybox", 0);
	m_shader->setUniform1i("uIrradianceMap", 1);
	m_shader->setUniform1i("uPrefilterMap", 2);
	m_shader->setUniform1i("uBRDFLUT", 3);

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

void Application::initSkybox()
{
	// Skybox init
	m_equirectangularToCubemapShader = std::make_unique<Shader>(RES_DIR"/shaders/equirectangular_vert.glsl", RES_DIR"/shaders/equirectangular_frag.glsl");
	m_irradianceShader = std::make_unique<Shader>(RES_DIR"/shaders/equirectangular_vert.glsl", RES_DIR"/shaders/irradiance_frag.glsl");
	m_prefilterShader = std::make_unique<Shader>(RES_DIR"/shaders/prefilter_vert.glsl", RES_DIR"/shaders/prefilter_frag.glsl");
	m_brdfShader = std::make_unique<Shader>(RES_DIR"/shaders/brdf_vert.glsl", RES_DIR"/shaders/brdf_frag.glsl");


	// Setup fbo
	glGenFramebuffers(1, &m_captureFBO);
	glGenRenderbuffers(1, &m_captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 2048, 2048);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_captureRBO);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::update()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// IBL textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_environment.envCubemap);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_environment.irradianceMap);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_environment.prefilterMap);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_brdfLUTTexture);

	m_shader->bind();
	m_shader->setUniformVec2f("uResolution", m_camera->getResolution());
	m_shader->setUniformVec3f("uLightPosition", m_lightPosition);
	m_shader->setUniform1f("uLightIntensity", m_lightIntensity);
	m_shader->setUniform1f("uExposure", m_exposure);
	m_shader->setUniformMat4f("uInverseViewProj", glm::inverse(m_camera->getProjectionMatrix() * m_camera->getViewMatrix()));
	m_shader->setUniformVec3f("uCameraPos", m_camera->getPosition());
	m_shader->setUniform1i("uAA", m_useAA);
	m_shader->setUniform1i("MAX_STEPS", m_maxSteps);


	for (int i = 0; i < m_spheres.size(); i++)
	{
		m_shader->setUniformVec3f("uSpheres[" + std::to_string(i) + "].center", m_spheres[i].center);
		m_shader->setUniform1f("uSpheres["+ std::to_string(i)+"].radius", m_spheres[i].radius);
		m_shader->setUniform1i("uSpheres["+ std::to_string(i)+"].tex", m_spheres[i].texture);
		m_shader->setUniformVec3f("uSpheres[" + std::to_string(i) + "].material.albedo", m_spheres[i].material.albedo);
		m_shader->setUniform1f("uSpheres[" + std::to_string(i) + "].material.metallic", m_spheres[i].material.metallic);
		m_shader->setUniform1f("uSpheres[" + std::to_string(i) + "].material.roughness", m_spheres[i].material.roughness);
		m_shader->setUniform1f("uSpheres[" + std::to_string(i) + "].material.ao", m_spheres[i].material.ao);
	}

	for (int i = 0; i < m_boxes.size(); i++)
	{
		m_shader->setUniformVec3f("uBoxes[" + std::to_string(i) + "].position", m_boxes[i].position);
		m_shader->setUniformVec3f("uBoxes[" + std::to_string(i) + "].b", m_boxes[i].b);
		m_shader->setUniform1f("uBoxes[" + std::to_string(i) + "].r", m_boxes[i].r);
		m_shader->setUniform1i("uBoxes[" + std::to_string(i) + "].tex", m_boxes[i].texture);
		m_shader->setUniformVec3f("uBoxes[" + std::to_string(i) + "].material.albedo", m_boxes[i].material.albedo);
		m_shader->setUniform1f("uBoxes[" + std::to_string(i) + "].material.metallic", m_boxes[i].material.metallic);
		m_shader->setUniform1f("uBoxes[" + std::to_string(i) + "].material.roughness", m_boxes[i].material.roughness);
		m_shader->setUniform1f("uBoxes[" + std::to_string(i) + "].material.ao", m_boxes[i].material.ao);
	}

	for (int i = 0; i < m_planes.size(); i++)
	{
		m_shader->setUniformVec3f("uPlanes[" + std::to_string(i) + "].n", m_planes[i].n);
		m_shader->setUniform1f("uPlanes[" + std::to_string(i) + "].d", m_planes[i].d);
		m_shader->setUniform1i("uPlanes[" + std::to_string(i) + "].tex", m_planes[i].texture);
		m_shader->setUniformVec3f("uPlanes[" + std::to_string(i) + "].material.albedo", m_planes[i].material.albedo);
		m_shader->setUniform1f("uPlanes[" + std::to_string(i) + "].material.metallic", m_planes[i].material.metallic);
		m_shader->setUniform1f("uPlanes[" + std::to_string(i) + "].material.roughness", m_planes[i].material.roughness);
		m_shader->setUniform1f("uPlanes[" + std::to_string(i) + "].material.ao", m_planes[i].material.ao);
	}

	m_shader->setUniform1i("uSceneOpCount", m_sceneOps.size());

	for (int i = 0; i < m_sceneOps.size(); ++i)
	{
		m_shader->setUniform1i("uSceneOps[" + std::to_string(i) + "].shapeType", m_sceneOps[i].shapeType);
		m_shader->setUniform1i("uSceneOps[" + std::to_string(i) + "].shapeIndex", m_sceneOps[i].shapeIndex);
		m_shader->setUniform1i("uSceneOps[" + std::to_string(i) + "].opType", m_sceneOps[i].opType);
	}

	glBindVertexArray(m_fullscreenVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void Application::updateUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGuizmo::BeginFrame();

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
	ImGui::SeparatorText("Shapes");
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);

	for (int i = 0; i<m_sceneOps.size(); i++)
	{
		auto& sceneOp = m_sceneOps[i];
		switch (sceneOp.shapeType)
		{
		case SHAPE_SPHERE:
		{
			auto& sphere = m_spheres[sceneOp.shapeIndex];
			if (sceneOp.name == "") sceneOp.name = "Sphere " + std::to_string(sceneOp.shapeIndex + 1);
			std::string& name = sceneOp.name;

			ImGui::PushID(i);

			if (ImGui::TreeNode(name.c_str()))
			{
				ImGui::DragFloat3("Position", glm::value_ptr(sphere.center), 0.1f);
				ImGui::DragFloat("Radius", &sphere.radius, 0.1f);

				if (ImGui::TreeNode("Material"))
				{
					ImGui::ColorPicker3("Albedo", glm::value_ptr(sphere.material.albedo));
					ImGui::DragFloat("Metallic", &sphere.material.metallic, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Roughness", &sphere.material.roughness, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Ambient Occlusion", &sphere.material.ao, 0.01f, 0.0f, 1.0f);
					ImGui::TreePop();
				}


				selectShape(i);

				if (ImGui::Button("Delete"))
				{
					eraseShape(SHAPE_SPHERE, sceneOp.shapeIndex, i);
					ImGui::TreePop();
					ImGui::PopID();
					break;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
			break;
		}
		case SHAPE_BOX:
		{
			auto& box = m_boxes[sceneOp.shapeIndex];
			if (sceneOp.name == "") sceneOp.name = "Box " + std::to_string(sceneOp.shapeIndex + 1);
			std::string& name = sceneOp.name;

			ImGui::PushID(i);

			if (ImGui::TreeNode(name.c_str()))
			{
				ImGui::DragFloat3("Position", glm::value_ptr(box.position), 0.1f);
				ImGui::DragFloat3("Half-Size", glm::value_ptr(box.b), 0.1f);
				ImGui::DragFloat("Radius", &box.r, 0.1f);

				if (ImGui::TreeNode("Material"))
				{
					ImGui::ColorPicker3("Albedo", glm::value_ptr(box.material.albedo));
					ImGui::DragFloat("Metallic", &box.material.metallic, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Roughness", &box.material.roughness, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Ambient Occlusion", &box.material.ao, 0.01f, 0.0f, 1.0f);
					ImGui::TreePop();
				}

				selectShape(i);

				if (ImGui::Button("Delete"))
				{
					eraseShape(SHAPE_BOX, sceneOp.shapeIndex, i);
					ImGui::TreePop();
					ImGui::PopID();
					break;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
			break;
		}
		case SHAPE_PLANE:
		{
			auto& plane = m_planes[sceneOp.shapeIndex];
			if (sceneOp.name == "") sceneOp.name = "Plane " + std::to_string(sceneOp.shapeIndex + 1);
			std::string& name = sceneOp.name;

			ImGui::PushID(i);

			if (ImGui::TreeNode(name.c_str()))
			{
				ImGui::DragFloat3("Normal", glm::value_ptr(plane.n), 0.1f, 0.0f, 1.0f);
				ImGui::DragFloat("Offset", &plane.d, 0.1f);

				if (ImGui::TreeNode("Material"))
				{
					ImGui::ColorPicker3("Albedo", glm::value_ptr(plane.material.albedo));
					ImGui::DragFloat("Metallic", &plane.material.metallic, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Roughness", &plane.material.roughness, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Ambient Occlusion", &plane.material.ao, 0.01f, 0.0f, 1.0f);
					ImGui::TreePop();
				}

				if (ImGui::Button("Delete"))
				{
					eraseShape(SHAPE_PLANE, sceneOp.shapeIndex, i);
					ImGui::TreePop();
					ImGui::PopID();
					break;
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
			break;
		}
		default: break;
		}
	}

	if (ImGui::Button("Add shape"))
	{
		ImGui::OpenPopup("Add Shape");
	}

	if (ImGui::BeginPopupModal("Add Shape", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static int shapeType = SHAPE_SPHERE;
		static int opType = OP_UNION;

		ImGui::Text("Create new shape");
		ImGui::Separator();

		ImGui::Text("Shape Type");
		ImGui::RadioButton("Sphere", &shapeType, SHAPE_SPHERE);
		ImGui::SameLine();
		ImGui::RadioButton("Box", &shapeType, SHAPE_BOX);
		ImGui::SameLine();
		ImGui::RadioButton("Plane", &shapeType, SHAPE_PLANE);

		ImGui::Separator();

		if (m_sceneOps.empty())
		{
			ImGui::Text("This will be the root shape.");
		}
		else
		{
			ImGui::Text("Operation");
			ImGui::RadioButton("Union", &opType, OP_UNION);
			ImGui::SameLine();
			ImGui::RadioButton("Smooth Union", &opType, OP_SMOOTH_UNION);
			ImGui::RadioButton("Intersection", &opType, OP_INTERSECTION);
			ImGui::SameLine();
			ImGui::RadioButton("Subtraction", &opType, OP_SUBTRACTION);
			ImGui::RadioButton("Smooth Subtraction", &opType, OP_SMOOTH_SUBTRACTION);

			ImGui::Separator();
		}

		if (shapeType == SHAPE_SPHERE)
		{
			ImGui::DragFloat3("Center", glm::value_ptr(sphereCenter), 0.1f);
			ImGui::DragFloat("Radius", &sphereRadius, 0.1f, 0.01f);
			ImGui::Checkbox("Checker Texture", reinterpret_cast<bool*>(&sphereTex));
		}
		else if (shapeType == SHAPE_BOX)
		{
			ImGui::DragFloat3("Position", glm::value_ptr(boxPosition), 0.1f);
			ImGui::DragFloat3("Half-Size", glm::value_ptr(boxHalfSize), 0.1f);
			ImGui::DragFloat("Round Radius", &boxRoundRadius, 0.01f, 0.0f);
			ImGui::Checkbox("Checker Texture", reinterpret_cast<bool*>(&boxTex));
		}
		else if (shapeType == SHAPE_PLANE)
		{
			ImGui::DragFloat3("Normal", glm::value_ptr(planeNormal), 0.1f);
			ImGui::DragFloat("Offset", &planeOffset, 0.1f);
			ImGui::Checkbox("Checker Texture", reinterpret_cast<bool*>(&planeTex));
		}

		ImGui::Separator();

		if (ImGui::Button("Create"))
		{
			createShape(shapeType, opType);

			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::SeparatorText("Light");
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
	ImGui::DragFloat3("Position", glm::value_ptr(m_lightPosition), 0.1f);
	ImGui::DragFloat("Intensity", &m_lightIntensity, 0.1f, 0.0f, 10.0); 
	ImGui::DragFloat("Exposure", &m_exposure, 0.01f, 0.01f, 10.0f);
	ImGui::SeparatorText("Raymarching settings");
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
	ImGui::Checkbox("Anti-Aliasing", &m_useAA);
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
	ImGui::DragInt("Max steps", &m_maxSteps, 1, 10, 500);

	updateGizmo();
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

void Application::createShape(int shapeType, int opType)
{
	if (shapeType == SHAPE_SPHERE)
	{
		m_spheres.push_back({ sphereCenter, sphereRadius, sphereTex });
		int shapeIndex = (int)m_spheres.size() - 1;

		m_sceneOps.push_back({
			SHAPE_SPHERE,
			shapeIndex,
			m_sceneOps.empty() ? OP_UNION : opType
			});
	}
	else if (shapeType == SHAPE_BOX)
	{
		m_boxes.push_back({ boxPosition, boxHalfSize, boxRoundRadius, boxTex });
		int shapeIndex = (int)m_boxes.size() - 1;

		m_sceneOps.push_back({
			SHAPE_BOX,
			shapeIndex,
			m_sceneOps.empty() ? OP_UNION : opType
			});
	}
	else if (shapeType == SHAPE_PLANE)
	{
		m_planes.push_back({ planeNormal, planeOffset, planeTex });
		int shapeIndex = (int)m_planes.size() - 1;

		m_sceneOps.push_back({
			SHAPE_PLANE,
			shapeIndex,
			m_sceneOps.empty() ? OP_UNION : opType
			});
	}
}

void Application::eraseShape(int shapeType, int shapeIndex, int sceneOpIndex)
{
	if (m_selectedShape.valid)
	{
		if (m_selectedShape.opIndex == sceneOpIndex)
		{
			m_selectedShape.valid = false;
			m_selectedShape.opIndex = -1;
		}
		else if (m_selectedShape.opIndex > sceneOpIndex)
		{
			m_selectedShape.opIndex--;
		}
	}

	m_sceneOps.erase(m_sceneOps.begin() + sceneOpIndex);

	switch (shapeType)
	{
	case SHAPE_SPHERE:
		m_spheres.erase(m_spheres.begin() + shapeIndex);
		break;

	case SHAPE_BOX:
		m_boxes.erase(m_boxes.begin() + shapeIndex);
		break;

	case SHAPE_PLANE:
		m_planes.erase(m_planes.begin() + shapeIndex);
		break;
	}

	for (auto& op : m_sceneOps)
	{
		if (op.shapeType == shapeType && op.shapeIndex > shapeIndex)
		{
			op.shapeIndex--;
		}
	}
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

	// Mouse scroll
	glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) {
		Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
		cam->scroll(yoffset);
	});

	// Window resize (using framebuffer)
	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
		Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
		glViewport(0, 0, width, height);
		cam->setResolution(width, height);
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

unsigned int Application::loadCubemap(const std::array<std::string, 6>& faces)
{
	unsigned int textureID = 0;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	stbi_set_flip_vertically_on_load(false);

	int width = 0;
	int height = 0;
	int channels = 0;

	for (unsigned int i = 0; i < faces.size(); ++i)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);
		if (!data)
		{
			std::cerr << "Failed to load cubemap face: " << faces[i] << std::endl;
			stbi_image_free(data);
			glDeleteTextures(1, &textureID);
			return 0;
		}

		GLenum format = GL_RGB;
		if (channels == 1) format = GL_RED;
		else if (channels == 3) format = GL_RGB;
		else if (channels == 4) format = GL_RGBA;

		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			format,
			width,
			height,
			0,
			format,
			GL_UNSIGNED_BYTE,
			data
		);

		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return textureID;
}

// Load and returns an HDR texture
unsigned int Application::loadHDRImage(const std::string path)
{
	unsigned int hdrTexture = 0;
	// Load radiance hdr map
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrChannels;
	float* data = stbi_loadf(path.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		glGenTextures(1, &hdrTexture);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);
		if (nrChannels == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
		}
		else if (nrChannels == 4) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, data);
		}
		else {
			std::cout << "Error: Unknown number of channels in hdr image" << std::endl;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load HDR image: " << path << std::endl;
		stbi_image_free(data);
	}
	return hdrTexture;
}

GLEnvironment Application::buildEnvironmentMaps(unsigned int hdrTexture)
{
	GLEnvironment environment{};
	environment.hdrTexture = hdrTexture;

	int maxMipLevels = 10;

	// Diffuse IBL
	// Create cubemap texture with hdr texture data
	glGenTextures(1, &environment.envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environment.envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
	};

	m_equirectangularToCubemapShader->bind();
	m_equirectangularToCubemapShader->setUniform1i("equirectangularMap", 0);
	m_equirectangularToCubemapShader->setUniformMat4f("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);

	glViewport(0, 0, 2048, 2048);
	glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		m_equirectangularToCubemapShader->setUniformMat4f("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environment.envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_width, window_height);

	glBindTexture(GL_TEXTURE_CUBE_MAP, environment.envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);


	// Create texture for convoluted irradiance cubemap
	glGenTextures(1, &environment.irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environment.irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

	// Create irradiance cubemap
	m_irradianceShader->bind();
	m_irradianceShader->setUniform1i("environmentMap", 0);
	m_irradianceShader->setUniformMat4f("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environment.envCubemap);

	glViewport(0, 0, 32, 32);
	glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		m_irradianceShader->setUniformMat4f("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environment.irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_width, window_height);

	// Specular IBL
	// Create texture for prefiltered environment map
	int specularSize = 512;
	glGenTextures(1, &environment.prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environment.prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, specularSize, specularSize, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// Generate mipmaps for the prefiltered environment map
	m_prefilterShader->bind();
	m_prefilterShader->setUniformMat4f("projection", captureProjection);
	m_prefilterShader->setUniform1i("environmentMap", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environment.envCubemap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		unsigned int mipWidth = specularSize * std::pow(0.5, mip);
		unsigned int mipHeight = specularSize * std::pow(0.5, mip);
		glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);
		float roughness = (float)mip / (float)(maxMipLevels - 1);
		m_prefilterShader->setUniform1f("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			m_prefilterShader->setUniformMat4f("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environment.prefilterMap, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderCube();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_width, window_height);

	// Generate BRDF LUT texture
	glGenTextures(1, &m_brdfLUTTexture);
	glBindTexture(GL_TEXTURE_2D, m_brdfLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_brdfLUTTexture, 0);

	glViewport(0, 0, 512, 512);
	m_brdfShader->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(m_fullscreenVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Reset viewport to window dimensions
	glViewport(0, 0, window_width, window_height);

	return environment;
}

glm::mat4 Application::getSelectedShapeTransform(const SceneOp& sceneOp) const
{
	if (!m_selectedShape.valid)
	{
		return glm::mat4(1.0f);
	}

	glm::vec3 position(0.0f);

	switch (sceneOp.shapeType)
	{
	case SHAPE_SPHERE:
		position = m_spheres[sceneOp.shapeIndex].center;
		break;

	case SHAPE_BOX:
		position = m_boxes[sceneOp.shapeIndex].position;
		break;

	default:
		break;
	}

	return glm::translate(glm::mat4(1.0f), position);
}

void Application::applySelectedShapeTransform(const glm::mat4& transform, const SceneOp& sceneOp)
{
	if (!m_selectedShape.valid)
	{
		return;
	}

	const glm::vec3 position = glm::vec3(transform[3]);

	switch (sceneOp.shapeType)
	{
	case SHAPE_SPHERE:
		m_spheres[sceneOp.shapeIndex].center = position;
		break;

	case SHAPE_BOX:
		m_boxes[sceneOp.shapeIndex].position = position;
		break;

	default:
		break;
	}
}

void Application::updateGizmo()
{
	if (!m_selectedShape.valid)
	{
		return;
	}

	auto& sceneOp = m_sceneOps[m_selectedShape.opIndex];

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

	ImGuiIO& io = ImGui::GetIO();

	ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);

	glm::mat4 view = m_camera->getViewMatrix();
	glm::mat4 projection = m_camera->getProjectionMatrix();
	glm::mat4 transform = getSelectedShapeTransform(sceneOp);

	ImGuizmo::Manipulate(
		glm::value_ptr(view),
		glm::value_ptr(projection),
		ImGuizmo::TRANSLATE,
		ImGuizmo::WORLD,
		glm::value_ptr(transform)
	);

	if (ImGuizmo::IsUsing())
	{
		applySelectedShapeTransform(transform, sceneOp);
	}
}

void Application::selectShape(int index)
{
	if (m_selectedShape.opIndex != index)
	{
		if (ImGui::Button("Select"))
		{
			m_selectedShape.valid = true;
			m_selectedShape.opIndex = index;
		}
	}
	else
	{
		if (ImGui::Button("Deselect"))
		{
			m_selectedShape.valid = false;
			m_selectedShape.opIndex = -1;
		}
	}
}
