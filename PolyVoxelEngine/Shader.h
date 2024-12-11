#pragma once
#include <glad/glad.h>
#include <unordered_map>
#include <glm/fwd.hpp>
#include <string>

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

