#include"Camera.h"

glm::vec3 Camera::GlobalUp = glm::vec3(0.0f, 1.0f, 0.0f);

Camera::Camera(glm::vec3 position, float fov, float near, float far) : position(position), nearPlane(near), farPlane(far)
{
	window = GraphicController::window;
	Fov = glm::radians(fov);
	updateFrustum();
}

void Camera::updateVectors()
{
	Forward = glm::normalize(Forward);
	Right = glm::normalize(glm::cross(Forward, GlobalUp));
	Up = glm::normalize(glm::cross(Right, Forward));
}

void Camera::updateMatrix()
{
	glm::mat4 view;
	glm::mat4 projection;

	view = glm::lookAt(position, position + Forward, Up);
	projection = glm::perspective(Fov, GraphicController::aspectRatio, nearPlane, farPlane);
	cameraMatrix = projection * view;

	updateFrustum();
}

void Camera::updateFrustum()
{
	float tanHF = tanf(Fov * 0.5f);
	float tanHFAR = tanHF * GraphicController::aspectRatio;

	float halfVSide = farPlane * tanHF;
	float halfHSide = halfVSide * GraphicController::aspectRatio;

	glm::vec3 frontMultFar = farPlane * Forward;

	frustum.near = Plane(position + nearPlane * Forward, Forward);
	frustum.far = Plane(position + frontMultFar, -Forward);

	frustum.right = Plane(position, glm::cross(frontMultFar - Right * halfHSide, Up));
	frustum.left = Plane(position, glm::cross(Up, frontMultFar + Right * halfHSide));

	frustum.top = Plane(position, glm::cross(Right, frontMultFar - Up * halfVSide));
	frustum.bottom = Plane(position, glm::cross(frontMultFar + Up * halfVSide, Right));
}

void Camera::passMatrixToShader(Shader* shader, const char* uniform) const
{
	shader->setUniformMat4(uniform, cameraMatrix);
}

void Camera::passPositionToShader(Shader* shader, const char* uniform) const
{
	shader->setUniformFloat3(uniform, position.x, position.y, position.z);
}

bool Camera::isOnFrustum(const Box& shape) const
{
	return 
	(
		isBoxOnOrForwardPlane(shape, frustum.near) &&
		isBoxOnOrForwardPlane(shape, frustum.far) &&
		isBoxOnOrForwardPlane(shape, frustum.right) &&
		isBoxOnOrForwardPlane(shape, frustum.left) &&
		isBoxOnOrForwardPlane(shape, frustum.top) &&
		isBoxOnOrForwardPlane(shape, frustum.bottom)
	);
}

bool Camera::isOnFrustum(const Sphere& shape) const
{
	return 
	(
		isSphereOnOrForwardPlane(shape, frustum.near)  &&
		isSphereOnOrForwardPlane(shape, frustum.far)   &&
		isSphereOnOrForwardPlane(shape, frustum.right) &&
		isSphereOnOrForwardPlane(shape, frustum.left)  &&
		isSphereOnOrForwardPlane(shape, frustum.top)   &&
		isSphereOnOrForwardPlane(shape, frustum.bottom)
	);
}
