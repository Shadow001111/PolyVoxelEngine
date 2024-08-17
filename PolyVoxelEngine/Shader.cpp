#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>

std::string ReadFile(const char* filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) {
		std::cout << "Failed opening file: " << filepath << std::endl;
		throw(errno);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

GLuint CreateShader(GLuint type, const char* filepath)
{
	GLuint shader = glCreateShader(type);
	std::string code = ReadFile(filepath);
	const char* source = code.c_str();
	glShaderSource(shader, 1, &source, NULL);
	return shader;
}

Shader::Shader(const std::string& name) : name(name)
{
	// shaders
	GLuint vertexShader = CreateShader(GL_VERTEX_SHADER, ("Shaders/" + name + ".vert").c_str());
	glCompileShader(vertexShader);
	checkForCompilationErrors(vertexShader, "VERTEX");

	GLuint fragmentShader = CreateShader(GL_FRAGMENT_SHADER, ("Shaders/" + name + ".frag").c_str());
	glCompileShader(fragmentShader);
	checkForCompilationErrors(fragmentShader, "FRAGMENT");

	// shader program
	ID = glCreateProgram();
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	glLinkProgram(ID);
	checkForCompilationErrors(ID, "PROGRAM");

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::bind() const
{
	glUseProgram(ID);
}

void Shader::unbind() const
{
	glUseProgram(0);
}

void Shader::clean() const
{
	glDeleteProgram(ID);
}



GLint Shader::getUniformPosition(const std::string& name)
{
	auto it = uniformCache.find(name);
	if (it == uniformCache.end()) {
		GLint position = glGetUniformLocation(ID, name.c_str());
		if (position == -1) {
			std::cout << "Uniform: " << name << " doesn't exist in: " << this->name << std::endl;
		}
		uniformCache[name] = position;
		return position;
	}
	return it->second;
}

void Shader::setUniformFloat(const std::string& name, float value)
{
	GLint position = getUniformPosition(name);
	glUniform1f(position, value);
}

void Shader::setUniformFloat2(const std::string& name, float v0, float v1)
{
	GLint position = getUniformPosition(name);
	glUniform2f(position, v0, v1);
}

void Shader::setUniformFloat3(const std::string& name, float v0, float v1, float v2)
{
	GLint position = getUniformPosition(name);
	glUniform3f(position, v0, v1, v2);
}

void Shader::setUniformFloat4(const std::string& name, float v0, float v1, float v2, float v3)
{
	GLint position = getUniformPosition(name);
	glUniform4f(position, v0, v1, v2, v3);
}

void Shader::setUniformInt(const std::string& name, int value)
{
	GLint position = getUniformPosition(name);
	glUniform1i(position, value);
}

void Shader::setUniformInt2(const std::string& name, int v0, int v1)
{
	GLint position = getUniformPosition(name);
	glUniform2i(position, v0, v1);
}

void Shader::setUniformMat4(const std::string& name, const glm::mat4& matrix)
{
	GLint position = getUniformPosition(name);
	glUniformMatrix4fv(position, 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::checkForCompilationErrors(unsigned int shader, const char* type) const
{
	GLint hasCompiled;

	char infolog[1024];
	if (type == "PROGRAM") {
		glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE) {
			glGetProgramInfoLog(shader, 1024, NULL, infolog);
			std::cout << "Failed to link shaders" << std::endl;
			std::cout << infolog << std::endl;
		}
	}
	else {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE) {
			glGetShaderInfoLog(shader, 1024, NULL, infolog);
			std::cout << "Failed to compile shader for:" << type << std::endl;
		}
	}
}