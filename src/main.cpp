#include <array>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "shader.h"
#include "camera.h"
#include "application.h"

#define width 1920
#define height 1080

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;


    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(width, height, "SDF", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // Load OpenGL functions using glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Hide cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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


    Camera camera(width, height, glm::vec3(0, 1, 0), 90.f, 0.f);

    Application app;
    app.camera = &camera;
    app.window = window;

    app.setCallbacks();

    // Create and compile our GLSL program from the shaders
    Shader shader(RES_DIR "/shaders/default_vert.glsl", RES_DIR"/shaders/sdf_frag.glsl");

    shader.bind();

    
    shader.setUniformVec2f("uResolution", camera.getResolution());


    shader.setUniformVec3f("uSpheres[0].center", glm::vec3(0,0,0));
    shader.setUniform1f("uSpheres[0].radius", 1.f);
	
	shader.setUniformVec3f("uSpheres[1].center", glm::vec3(3,0,0));
    shader.setUniform1f("uSpheres[1].radius", 0.5f);


    shader.setUniform1i("uSphereCount", 2);

    // Enable depth 
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable antialiasing
    glEnable(GL_MULTISAMPLE);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        app.deltaTime();
        app.processInput();

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.setUniformMat4f("uInverseViewProj", glm::inverse(camera.getProjectionMatrix() * camera.getViewMatrix()));
        shader.setUniformVec3f("uCameraPos", camera.getPosition());

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}