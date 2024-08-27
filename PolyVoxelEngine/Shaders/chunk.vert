#version 460 core

layout(location = 0) in vec3 vertPos;
#ifdef SMOOTH_LIGHTING
layout(location = 1) in ivec3 packedData;
#else
layout(location = 1) in ivec2 packedData;
#endif

layout(binding = 0) restrict readonly buffer ChunkPositionSSBO
{
	float chunkPositions[];
};
layout(binding = 1) restrict readonly buffer ChunkPositionIndexSSBO
{
	uint chunkPositionIndexes[];
};

#ifndef Z_PRE_PASS
const float aoValues[4] = float[4]
(
	0.1, 0.25, 0.75, 1.0
);
#endif

const float extending = 1.001;

uniform mat4 camMatrix;

#ifndef Z_PRE_PASS
uniform vec3 camPos;
uniform float dayNightCycleSkyLightingSubtraction;

out vec2 uv;
out vec2 normalizedUV;
flat out float textureID;
flat out float[4] gottenAOValues;
out float depth;
 #ifdef SMOOTH_LIGHTING
flat out float[4] gottenLightingValues;
 #endif
flat out float blockLight;
flat out float skyLight;
#endif

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

	#ifndef Z_PRE_PASS
	const int ao = (packedData.x >> 23) & 255;

	textureID = packedData.y & 255;
	blockLight = ((packedData.y >> 8) & 15) / 15.0;
	skyLight = max(0.0, round(((packedData.y >> 12) & 15) - dayNightCycleSkyLightingSubtraction * 15.0) / 15.0);

	 #ifdef SMOOTH_LIGHTING
	for (int i = 0; i < 4; i++)
	{
		int tempL = (packedData.z >> (i * 8)) & 255;
		float blockLight = tempL & 15;
		float skyLight = max(0.0, (tempL >> 4) - dayNightCycleSkyLightingSubtraction * 15.0);
		gottenLightingValues[i] = max(blockLight, skyLight) / 15.0f;
	}
	 #endif
	#endif
	//
	vec3 localPos = vertPos;
	#ifndef Z_PRE_PASS
	normalizedUV = localPos.xz;
	#endif
	localPos.xz *= unpackedSize;
	#ifndef Z_PRE_PASS
	uv = localPos.xz;
	#endif
	
	if (normalID == 0) // right
	{
		localPos.y = localPos.x;
		localPos.x = 1.0;
		localPos.z = unpackedSize.y - localPos.z;
		#ifndef Z_PRE_PASS
		uv = uv.yx;
		#endif

		const vec2 center = unpackedSize * 0.5f;
		localPos.yz = (localPos.yz - center) * extending + center;
	}
	else if (normalID == 1) // left
	{
		localPos.xy = localPos.yx;
		#ifndef Z_PRE_PASS
		uv = uv.yx;
		#endif

		const vec2 center = unpackedSize * 0.5f;
		localPos.yz = (localPos.yz - center) * extending + center;
	}
	else if (normalID == 2) // up
	{
		localPos.y = 1.0;
		#ifndef Z_PRE_PASS
		uv.y = unpackedSize.y - uv.y;
		#endif

		const vec2 center = unpackedSize * 0.5f;
		localPos.xz = (localPos.xz - center) * extending + center;
	}
	else if (normalID == 3) // down
	{
		localPos.x = unpackedSize.x - localPos.x;
		#ifndef Z_PRE_PASS
		uv.x = unpackedSize.x - uv.x;
		#endif

		const vec2 center = unpackedSize * 0.5f;
		localPos.xz = (localPos.xz - center) * extending + center;
	}
	else if (normalID == 4) // front
	{
		localPos.y = localPos.z;
		localPos.z = 1.0;
		localPos.x = unpackedSize.x - localPos.x;
		#ifndef Z_PRE_PASS
		uv.x = unpackedSize.x - uv.x;
		#endif

		const vec2 center = unpackedSize * 0.5f;
		localPos.xy = (localPos.xy - center) * extending + center;
	}
	else // back
	{
		localPos.zy = localPos.yz;
		#ifndef Z_PRE_PASS
		uv.x = unpackedSize.x - uv.x;
		#endif

		const vec2 center = unpackedSize * 0.5f;
		localPos.xy = (localPos.xy - center) * extending + center;
	}

	#ifndef Z_PRE_PASS
	gottenAOValues[0] = aoValues[ao & 3];
	gottenAOValues[1] = aoValues[(ao >> 2) & 3];
	gottenAOValues[2] = aoValues[(ao >> 4) & 3];
	gottenAOValues[3] = aoValues[ao >> 6];
	#endif

	const uint posIndex = chunkPositionIndexes[gl_DrawID] * 3;
	const vec3 globalChunkPos = vec3
	(
		chunkPositions[posIndex],
		chunkPositions[posIndex + 1],
		chunkPositions[posIndex + 2]
	);

	const vec3 vertexPos = localPos + unpackedPos + globalChunkPos;

	#ifndef Z_PRE_PASS
	const vec3 vertexCamPos = vertexPos - camPos;
	depth = length(vertexCamPos);
	#endif

	gl_Position = camMatrix * vec4(vertexPos, 1.0);
}