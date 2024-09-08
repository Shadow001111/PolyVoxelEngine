#pragma once
#include "World.h"

class PhysicEntity
{
	Chunk* chunk = nullptr;
	PhysicEntityCollider collider;
public:
	glm::vec3 position, previousPosition;
	glm::vec3 velocity;
	glm::vec3 colliderSize, colliderDpos;

	static World* world;

	bool isGrounded = false;

	bool gravityEnabled = true;
	bool collisionEnabled = true;
	float airResistanceMultiplier = 1.0f;

	PhysicEntity(glm::vec3 position, glm::vec3 colliderSize, glm::vec3 colliderDpos);

	void accelerate(const glm::vec3& acc);
	void physicUpdate(float dt);
};

