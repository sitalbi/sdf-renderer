#include "shader.h"
#include "glad/glad.h"
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const std::string& vertexFilePath, const std::string& fragmentFilePath)
    : m_vertexFilePath(vertexFilePath), m_fragmentFilePath(fragmentFilePath), m_shaderID(0)
{
    m_shaderID = compile();
    bind();
}

Shader::~Shader()
{
    glDeleteProgram(m_shaderID);
}

unsigned int Shader::compile() {
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(m_vertexFilePath, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }
    else {
        printf("Impossible to open %s.\n", m_vertexFilePath.c_str());
        getchar();
        return 0;
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(m_fragmentFilePath, std::ios::in);
    if (FragmentShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }
    else {
        printf("Impossible to open %s.\n", m_fragmentFilePath.c_str());
        getchar();
        return 0;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    printf("Compiling vertex shader\n");
    char const* VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    // Compile Fragment Shader
    printf("Compiling fragment shader\n");
    char const* FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }

    // Link the program
    printf("Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

int Shader::getUniformLocation(const std::string& name)
{
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
    {
        return m_UniformLocationCache[name];
    }
    else {
        int location = glGetUniformLocation(m_shaderID, name.c_str());
        m_UniformLocationCache[name] = location;
        return location;
    }
}

void Shader::bind() const
{
    glUseProgram(m_shaderID);
}

void Shader::unbind() const
{
    glUseProgram(0);
}

void Shader::setUniform1f(const std::string& name, float value)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniform1f(location, value);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniform2f(const std::string& name, float v0, float v1)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniform2f(location, v0, v1);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniform3f(const std::string& name, float v0, float v1, float v2)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniform3f(location, v0, v1, v2);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniform4f(location, v0, v1, v2, v3);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniformMat3f(const std::string& name, const glm::mat3& matrix)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniformMatrix3fv(location, 1, GL_FALSE, &matrix[0][0]);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniform1i(const std::string& name, int value)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniform1i(location, value);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniformMat4f(const std::string& name, const glm::mat4& matrix)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniformVec3f(const std::string& name, const glm::vec3& vector)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniform3fv(location, 1, &vector[0]);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniformBool(const std::string& name, bool value)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniform1i(location, value);
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}

void Shader::setUniform3fv(const std::string& name, const std::vector<glm::vec3> vector, int count)
{
    int location = getUniformLocation(name);
    if (location != -1)
        glUniform3fv(location, count, glm::value_ptr(vector[0]));
    else
        std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
}