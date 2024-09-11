#include "FaceInstancesVBO.h"
#include <glad/glad.h>

FaceInstancesVBO::FaceInstancesVBO(size_t instancesCount, size_t layoutOffset)
{
    autolinkLayout = layoutOffset;

    glGenBuffers(1, &ID);
    glBindBuffer(GL_ARRAY_BUFFER, ID);

    glBufferData(GL_ARRAY_BUFFER, instancesCount * sizeof(FaceInstanceData), nullptr, GL_DYNAMIC_DRAW);
#if ENABLE_SMOOTH_LIGHTING
    linkInt(3);
#else
    linkInt(2);
#endif

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FaceInstancesVBO::setData(const FaceInstanceData* instancesData, size_t offset, size_t count)
{
    glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(FaceInstanceData), count * sizeof(FaceInstanceData), instancesData);
}

void FaceInstancesVBO::linkFloat(unsigned int num_components)
{
    unsigned int layout = autolinkLayout++;
    glVertexAttribPointer(layout, num_components, GL_FLOAT, GL_FALSE, sizeof(FaceInstanceData), (void*)autolinkOffset);
    glEnableVertexAttribArray(layout);
    glVertexAttribDivisor(layout, 1);
    autolinkOffset += num_components * sizeof(GLfloat);
}

void FaceInstancesVBO::linkInt(int num_components)
{
    unsigned int layout = autolinkLayout++;
    glVertexAttribIPointer(layout, num_components, GL_INT, sizeof(FaceInstanceData), (void*)autolinkOffset);
    glEnableVertexAttribArray(layout);
    glVertexAttribDivisor(layout, 1);
    autolinkOffset += num_components * sizeof(GLint);
}

void FaceInstancesVBO::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void FaceInstancesVBO::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FaceInstancesVBO::clean() const
{
    glDeleteBuffers(1, &ID);
}

FaceInstanceData::FaceInstanceData()
{}

#if ENABLE_SMOOTH_LIGHTING
void FaceInstanceData::set(int x, int y, int z, int w, int h, int normalID, char ao, unsigned int textureID, int lighting, const uint8_t* softLighting)
{
    data1 = x | (y << 4) | (z << 8) | ((w - 1) << 12) | ((h - 1) << 16) |
        (normalID << 20) | (ao << 23); // 31 bits
    data2 = textureID | (lighting << 8); // 16 bits
    data3 = softLighting[0] | (softLighting[1] << 8) | (softLighting[2] << 16) | (softLighting[3] << 24);
}
#else
void FaceInstanceData::set(int x, int y, int z, int w, int h, int normalID, char ao, unsigned int textureID, int lighting)
{
    data1 = x | (y << 4) | (z << 8) | ((w - 1) << 12) | ((h - 1) << 16) |
        (normalID << 20) | (ao << 23); // 31 bits
    data2 = textureID | (lighting << 8); // 16 bits
}
#endif