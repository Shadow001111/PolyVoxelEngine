#pragma once
#include<glad/glad.h>
#include "GraphicController.h"
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtx/rotate_vector.hpp>
#include<glm/gtx/vector_angle.hpp>
#include "Shader.h"
#include "Shapes.h"

constexpr glm::vec3 GLOBAL_UP(0.0f, 1.0f, 0.0f);

class Camera
{
	struct Frustum
	{
		Plane top;
		Plane bottom;
		Plane right;
		Plane left;
		Plane far;
		Plane near;

		Frustum() {};
	};

	static glm::vec3 GlobalUp;
	glm::mat4 cameraMatrix = glm::mat4(1.0f);

	Frustum frustum;

	float Fov;
	float nearPlane;
	float farPlane;
	GLFWwindow* window;

	void updateFrustum();
public:
	glm::vec3 position;
	glm::vec3 Forward = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

	Camera(glm::vec3 position, float fov, float near, float far);
	
	void updateVectors();
	void updateMatrix();

	void passMatrixToShader(Shader* shader, const char* uniform) const;
	void passPositionToShader(Shader* shader, const char* uniform) const;

	bool isOnFrustum(const Box& shape) const;
	bool isOnFrustum(const Sphere& shape) const;
};

