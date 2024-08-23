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

const float aoValues[4] = float[4]
(
	0.1, 0.25, 0.75, 1.0
);

const float extending = 1.001;

uniform mat4 camMatrix;
uniform vec3 camPos;
uniform float dayNightCycleSkyLightingSubtraction;

out vec2 uv;
out vec2 normalizedUV;
flat out float textureID;
out flat float[4] gottenAOValues;
out float depth;
flat out float blockLight;
flat out float skyLight;

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

	textureID = packedData.y & 255;
	blockLight = ((packedData.y >> 8) & 15) / 15.0;
	skyLight = max(0.0, round(((packedData.y >> 12) & 15) - dayNightCycleSkyLightingSubtraction * 15.0) / 15.0);

	//
	vec3 localPos = vertPos;
	normalizedUV = localPos.xz;
	localPos.xz *= unpackedSize;
	uv = localPos.xz;
	
	if (normalID == 0) // right
	{
		localPos.y = localPos.x;
		localPos.x = 1.0;
		localPos.z = unpackedSize.y - localPos.z;
		uv = uv.yx;

		const vec2 center = unpackedSize * 0.5f;
		localPos.yz = (localPos.yz - center) * extending + center;
	}
	else if (normalID == 1) // left
	{
		localPos.xy = localPos.yx;
		uv = uv.yx;

		const vec2 center = unpackedSize * 0.5f;
		localPos.yz = (localPos.yz - center) * extending + center;
	}
	else if (normalID == 2) // up
	{
		localPos.y = 1.0;
		uv.y = unpackedSize.y - uv.y;

		const vec2 center = unpackedSize * 0.5f;
		localPos.xz = (localPos.xz - center) * extending + center;
	}
	else if (normalID == 3) // down
	{
		localPos.x = unpackedSize.x - localPos.x;
		uv.x = unpackedSize.x - uv.x;

		const vec2 center = unpackedSize * 0.5f;
		localPos.xz = (localPos.xz - center) * extending + center;
	}
	else if (normalID == 4) // front
	{
		localPos.y = localPos.z;
		localPos.z = 1.0;
		localPos.x = unpackedSize.x - localPos.x;
		uv.x = unpackedSize.x - uv.x;

		const vec2 center = unpackedSize * 0.5f;
		localPos.xy = (localPos.xy - center) * extending + center;
	}
	else // back
	{
		localPos.zy = localPos.yz;
		uv.x = unpackedSize.x - uv.x;

		const vec2 center = unpackedSize * 0.5f;
		localPos.xy = (localPos.xy - center) * extending + center;
	}

	gottenAOValues[0] = aoValues[ao & 3];
	gottenAOValues[1] = aoValues[(ao >> 2) & 3];
	gottenAOValues[2] = aoValues[(ao >> 4) & 3];
	gottenAOValues[3] = aoValues[ao >> 6];

	const uint posIndex = chunkPositionIndexes[gl_DrawID] * 3;
	const vec3 globalChunkPos = vec3
	(
		chunkPositions[posIndex],
		chunkPositions[posIndex + 1],
		chunkPositions[posIndex + 2]
	);

	const vec3 vertexPos = localPos + unpackedPos + globalChunkPos;

	const vec3 vertexCamPos = vertexPos - camPos;
	depth = length(vertexCamPos);

	gl_Position = camMatrix * vec4(vertexPos, 1.0);
}