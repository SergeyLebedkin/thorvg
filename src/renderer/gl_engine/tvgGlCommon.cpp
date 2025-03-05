/*
 * Copyright (c) 2023 - 2025 the ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "tvgGlCommon.h"

void GlContext::initialize()
{
    preferredFormat = GL_RGBA8;
    samplerNearestRepeat = createSampler(GL_NEAREST, GL_REPEAT);
    samplerLinearRepeat = createSampler(GL_LINEAR, GL_REPEAT);
    samplerLinearMirror = createSampler(GL_LINEAR, GL_MIRRORED_REPEAT);
    samplerLinearClamp = createSampler(GL_LINEAR, GL_CLAMP_TO_EDGE);
    assert(samplerNearestRepeat);
    assert(samplerLinearRepeat);
    assert(samplerLinearMirror);
    assert(samplerLinearClamp);
}


GLuint GlContext::createSampler(GLint filter, GLint wrapMode)
{
    GLuint sampler{};
    GL_CHECK(glGenSamplers(1, &sampler));
    GL_CHECK(glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, filter));
    GL_CHECK(glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, filter));
    GL_CHECK(glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, wrapMode));
    GL_CHECK(glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, wrapMode));
    return sampler;
}


GLuint GlContext::createBuffer()
{
    GLuint buffer{};
    glGenBuffers(1, &buffer);
    return buffer;
}


bool GlContext::allocateTexture(GLuint& texture, GLsizei width, GLsizei height, GLint format, void* data)
{
    GLuint prev = texture;
    if (!texture) { GL_CHECK(glGenTextures(1, &texture)); }
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_BYTE, data));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    return prev != texture;
}


bool GlContext::allocateBufferVertex(GLuint& buffer, const float* data, uint64_t size)
{
    GLuint prev = buffer;
    if (!buffer) { GL_CHECK(glGenBuffers(1, &buffer)); }
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffer));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    return prev != buffer;
}


bool GlContext::allocateBufferIndex(GLuint& buffer, const uint32_t* data, uint64_t size)
{
    GLuint prev = buffer;
    if (!buffer) { GL_CHECK(glGenBuffers(1, &buffer)); }
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    return prev != buffer;
}


void GlContext::release()
{
    releaseSampler(samplerLinearClamp);
    releaseSampler(samplerLinearMirror);
    releaseSampler(samplerLinearRepeat);
    releaseSampler(samplerNearestRepeat);
}


void GlContext::releaseTexture(GLuint& texture)
{
    if (texture) {
        GL_CHECK(glDeleteTextures(1, &texture));
        texture = 0;
    }
}


void GlContext::releaseSampler(GLuint& sampler)
{
    if (sampler) {
        GL_CHECK(glDeleteSamplers(1, &sampler));
        sampler = 0;
    }
}


void GlContext::releaseBuffer(GLuint& buffer)
{
    if (buffer) {
        GL_CHECK(glDeleteSamplers(1, &buffer));
        buffer = 0;
    }
}


void GlContext::releaseVertexArray(GLuint& vertexArray)
{
    if (vertexArray) {
        GL_CHECK(glDeleteVertexArrays(1, &vertexArray));
        vertexArray = 0;
    }
}