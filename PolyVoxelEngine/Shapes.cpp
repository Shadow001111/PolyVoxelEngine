#include "Shapes.h"

Box::Box() : center(0.0f, 0.0f, 0.0f), extents(0.0f, 0.0f, 0.0f)
{}
Box::Box(glm::vec3 center, glm::vec3 extents) : center(center), extents(extents) 
{}

//Sphere::Sphere() : center(0.0f, 0.0f, 0.0f), radius(0.0f)
//{}
//Sphere::Sphere(glm::vec3 center, float radius) : center(center), radius(radius)
//{}

Plane::Plane() : center(0.0f, 0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f)
{}
Plane::Plane(glm::vec3 center, glm::vec3 normal) : center(center), normal(glm::normalize(normal)) 
{}

inline float distanceToPlane(const glm::vec3& point, const Plane& plane)
{
	return glm::dot(plane.normal, point - plane.center);
}

//bool isSphereOnOrForwardPlane(const Sphere& sphere, const Plane& plane)
//{
//	return distanceToPlane(sphere.center, plane) > -sphere.radius;
//}

bool isBoxOnOrForwardPlane(const Box& box, const Plane& plane)
{
	return distanceToPlane(box.center, plane) >= -glm::dot(box.extents, glm::abs(plane.normal));
}