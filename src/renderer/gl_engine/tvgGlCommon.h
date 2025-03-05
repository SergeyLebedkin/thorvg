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

#ifndef _TVG_GL_COMMON_H_
#define _TVG_GL_COMMON_H_

#include <cassert>
#include <tvgCommon.h>

#if defined (THORVG_GL_TARGET_GLES)
    #include <GLES3/gl3.h>
    #define TVG_REQUIRE_GL_MAJOR_VER 3
    #define TVG_REQUIRE_GL_MINOR_VER 0
#else
    #if defined(__APPLE__) || defined(__MACH__)
        #include <OpenGL/gl3.h>
    #else
        #define GL_GLEXT_PROTOTYPES 1
        #include <GL/gl.h>
    #endif
    #define TVG_REQUIRE_GL_MAJOR_VER 3
    #define TVG_REQUIRE_GL_MINOR_VER 3
#endif

#ifdef __EMSCRIPTEN__
    #include <emscripten/html5_webgl.h>
    // query GL Error on WebGL is very slow, so disable it on WebGL
    #define GL_CHECK(x) x
#else
    #define GL_CHECK(stmt) stmt; assert(glGetError() == GL_NO_ERROR);
#endif

struct GlContext {
    GLenum preferredFormat{};
    // shared opengl assets
    GLuint samplerNearestRepeat{};
    GLuint samplerLinearRepeat{};
    GLuint samplerLinearMirror{};
    GLuint samplerLinearClamp{};

    void initialize();
    void release();

    GLuint createSampler(GLint filter, GLint wrapMode);
    GLuint createBuffer();

    bool allocateTexture(GLuint& texture, GLsizei width, GLsizei height, GLint format, void* data);
    bool allocateBufferVertex(GLuint& buffer, const float* data, uint64_t size);
    bool allocateBufferIndex(GLuint& buffer, const uint32_t* data, uint64_t size);

    void releaseTexture(GLuint& texture);
    void releaseSampler(GLuint& sampler);
    void releaseBuffer(GLuint& buffer);
    void releaseVertexArray(GLuint& vertexArray);
};

#endif // _TVG_GL_COMMON_H_
