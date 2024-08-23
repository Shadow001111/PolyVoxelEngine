#version 460 core

layout(location = 0) in vec3 vertPos;
layout(location = 1) in ivec2 packedData;


layout(binding = 0) buffer ChunkPositionSSBO
{
	float chunkPositions[];
};
layout(binding = 1) buffer ChunkPositionIndexSSBO
{
	uint chunkPositionIndexes[];
};

const vec3 normals[6] = vec3[6]
(
	vec3(1.0, 0.0, 0.0), // right
	vec3(-1.0, 0.0, 0.0), // left
	vec3(0.0, 1.0, 0.0), // up
	vec3(0.0, -1.0, 0.0), // down
	vec3(0.0, 0.0, 1.0), // front
	vec3(0.0, 0.0, -1.0) // back
);

const float extending = 1.001;

uniform mat4 camMatrix;

void main()
{
	// unpack data
	const vec3 unpackedPos = vec3
	(
		packedData.x & 15,
		(packedData.x >> 4) & 15,
		(packedData.x >> 8) & 15
	);
	const vec2 unpackedSize = vec2
	(
		((packedData.x >> 12) & 15) + 1,
		((packedData.x >> 16) & 15) + 1
	);
	const int normalID = (packedData.x >> 20) & 7;
	const int ao = (packedData.x >> 23) & 255;

	//
	vec3 localPos = vertPos;
	localPos.xz *= unpackedSize;
	
	if (normalID == 0) // right
	{
		localPos.y = localPos.x;
		localPos.x = 1.0;
		localPos.z = unpackedSize.y - localPos.z;

		const vec2 center = unpackedSize * 0.5f;
		localPos.yz = (localPos.yz - center) * extending + center;
	}
	else if (normalID == 1) // left
	{
		localPos.xy = localPos.yx;

		const vec2 center = unpackedSize * 0.5f;
		localPos.yz = (localPos.yz - center) * extending + center;
	}
	else if (normalID == 2) // up
	{
		localPos.y = 1.0;

		const vec2 center = unpackedSize * 0.5f;
		localPos.xz = (localPos.xz - center) * extending + center;
	}
	else if (normalID == 3) // down
	{
		localPos.x = unpackedSize.x - localPos.x;

		const vec2 center = unpackedSize * 0.5f;
		localPos.xz = (localPos.xz - center) * extending + center;
	}
	else if (normalID == 4) // front
	{
		localPos.y = localPos.z;
		localPos.z = 1.0;
		localPos.x = unpackedSize.x - localPos.x;

		const vec2 center = unpackedSize * 0.5f;
		localPos.xy = (localPos.xy - center) * extending + center;
	}
	else // back
	{
		localPos.zy = localPos.yz;

		const vec2 center = unpackedSize * 0.5f;
		localPos.xy = (localPos.xy - center) * extending + center;
	}

	const uint posIndex = chunkPositionIndexes[gl_DrawID] * 3;
	const vec3 globalChunkPos = vec3
	(
		chunkPositions[posIndex],
		chunkPositions[posIndex + 1],
		chunkPositions[posIndex + 2]
	);

	const vec3 vertexPos = localPos + unpackedPos + globalChunkPos;

	gl_Position = camMatrix * vec4(vertexPos, 1.0);
}