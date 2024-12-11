#include "PhysicEntity.h"

const glm::vec3 PhysicEntity::GLOBAL_UP = glm::vec3(0.0f, 1.0f, 0.0f);
World* PhysicEntity::world = nullptr;

PhysicEntity::PhysicEntity(glm::vec3 position, glm::vec3 colliderSize, glm::vec3 colliderDpos)
	: collider(this->position, this->colliderSize, this->colliderDpos), position(position), previousPosition(position), velocity(0.0f), colliderSize(colliderSize * 0.5f), colliderDpos(colliderDpos)
{}

void PhysicEntity::accelerate(const glm::vec3 & acc)
{
	velocity += acc;
}

void PhysicEntity::physicUpdate(float dt)
{
	previousPosition = position;
	
	// gravity
	if (gravityEnabled)
	{
		accelerate(Settings::GRAVITY * dt * GLOBAL_UP);
	}

	// airResistance
	{
		float speed = glm::length(velocity);
		if (speed > 0.0f)
		{
			float force = fminf(Settings::AIR_RESISTANCE * airResistanceMultiplier * speed * dt, speed);
			velocity -= (velocity / speed) * force;;
		}
	}

	// velocity y clamp
	velocity.y = fminf(Settings::MAX_Y_SPEED, fmaxf(-Settings::MAX_Y_SPEED, velocity.y));

	// friction
	if (isGrounded)
	{
		float factor = powf(0.05f, dt);
		velocity.x *= factor;
		velocity.z *= factor;
	}

	// movement and collision
	glm::vec3 dpos = velocity * dt;

	if (collisionEnabled)
	{
		glm::uvec3 axisPointSamples = glm::ceil(colliderSize) + 1.0f;
		glm::uvec3 axisCheckTimes = glm::ceil(glm::abs(dpos));
		isGrounded = false;

		glm::vec3 colliderPosition = position + colliderDpos;

		for (size_t axis = 0; axis < 3; axis++)
		{
			size_t j = (axis + 1) % 3;
			size_t k = (j + 1) % 3;
			float jIncr = colliderSize[j] * 2.0f / (axisPointSamples[j] - 1);
			float kIncr = colliderSize[k] * 2.0f / (axisPointSamples[k] - 1);

			glm::vec3 axisSize = { 0.0f, 0.0f, 0.0f };
			axisSize[axis] = colliderSize[axis];

			bool isColliding = false;
			float dposStep = dpos[axis] / axisCheckTimes[axis];
			bool dposStepSign = dposStep > 0.0f;
			for (unsigned int axisCheckI = 0; (axisCheckI < axisCheckTimes[axis]) && !isColliding; axisCheckI++)
			{
				colliderPosition[axis] += dposStep;

				isColliding = false;
				for (unsigned int j_ = 0; (j_ < axisPointSamples[j]) && !isColliding; j_++)
				{
					axisSize[j] = -colliderSize[j] + j_ * jIncr;
					for (unsigned int k_ = 0; k_ < axisPointSamples[k]; k_++)
					{
						axisSize[k] = -colliderSize[k] + k_ * kIncr;

						glm::ivec3 voxelPos;
						if (dposStepSign)
						{
							voxelPos = glm::floor(colliderPosition + axisSize);
						}
						else
						{
							voxelPos = glm::floor(colliderPosition - axisSize);
						}

						Block block = world->getBlockAt(voxelPos.x, voxelPos.y, voxelPos.z);
						if (ALL_BLOCK_DATA[(size_t)block].colliding)
						{
							isColliding = true;
							break;
						}
					}
				}
			}

			if (isColliding)
			{
				if (dposStepSign)
				{
					colliderPosition[axis] = floorf(colliderPosition[axis] + colliderSize[axis]) - (colliderSize[axis] + 1e-3f);
				}
				else
				{
					colliderPosition[axis] = ceilf(colliderPosition[axis] - colliderSize[axis]) + (colliderSize[axis] + 1e-3f);
					if (axis == 1)
					{
						isGrounded = true;
					}
				}
				velocity[axis] = 0.0f;
			}
		}

		position = colliderPosition - colliderDpos;
	}
	else
	{
		position += dpos;
	}

	// move to other chunk
	{
		glm::ivec3 chunkPos = glm::floor(position / floorf(Settings::CHUNK_SIZE));
		Chunk* newChunk = Chunk::getChunkAt(chunkPos.x, chunkPos.y, chunkPos.z);
		if (chunk != newChunk)
		{
			if (chunk)
			{
				chunk->physicEntities.remove(&collider);
			}
			chunk = newChunk;
			if (chunk)
			{
				chunk->physicEntities.push(&collider);
			}
			else
			{
				std::cerr << "Entity entered non-chunk" << std::endl;
			}
		}
	}
}
