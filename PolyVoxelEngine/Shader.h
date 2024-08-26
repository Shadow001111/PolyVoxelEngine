#pragma once
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <unordered_map>
#include <glm/glm.hpp>

std::vector<std::string> splitString(const std::string& input, char delimiter = ' ');

std::string ReadFile(const char* filepath);
GLuint CreateShader(GLuint type, const char* filepath, const std::vector<std::string>& flags);

class Shader
{
	std::string name;
public:
	GLuint ID;
	Shader(const std::string& name, const std::string& flags = "");

	void bind() const;
	void unbind() const;
	void clean() const;

	void setUniformFloat(const std::string& name, float value);
	void setUniformFloat2(const std::string& name, float v0, float v1);
	void setUniformFloat3(const std::string& name, float v0, float v1, float v2);
	void setUniformFloat4(const std::string& name, float v0, float v1, float v2, float v3);

	void setUniformInt(const std::string& name, int value);
	void setUniformInt2(const std::string& name, int v0, int v1);
	void setUniformMat4(const std::string& name, const glm::mat4& matrix);
private:
	std::unordered_map<std::string, GLuint> uniformCache;

	GLint getUniformPosition(const std::string& name);

	void checkForCompilationErrors(unsigned int shader, const char* type) const;
};

