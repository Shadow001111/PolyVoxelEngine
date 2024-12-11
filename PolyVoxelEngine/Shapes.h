#pragma once
#include <glm/vec3.hpp>

struct Box
{
	glm::vec3 center, extents;

	Box();
	Box(glm::vec3 center, glm::vec3 extents);
};

//struct Sphere
//{
//	glm::vec3 center;
//	float radius;
//
//	Sphere();
//	Sphere(glm::vec3 center, float radius);
//};

struct Plane
{
	glm::vec3 center, normal;

	Plane();
	Plane(glm::vec3 center, glm::vec3 normal);

	float distanceToPoint(const glm::vec3& point) const;
};

//bool isSphereOnOrForwardPlane(const Sphere& sphere, const Plane& plane);
bool isBoxOnOrForwardPlane(const Box& box, const Plane& plane);