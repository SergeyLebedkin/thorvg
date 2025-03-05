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

#ifndef _TVG_GL_SHADERS_H_
#define _TVG_GL_SHADERS_H_
 
#include "tvgGlCommon.h"

// uniform locations
struct GlUniformLocations
{
    // uniforms
    GLint uViewMat{};
    GLint uModelMat{};
    GLint uColorDebug{};
    // textures
    GLint uImageSrc{};
};

struct GlProgram
{
    GLuint prog{};
    GlUniformLocations uloc{};

    void initialize(GlContext& context, GLuint vertexShader, GLuint fragmentShader);
    void release(GlContext& context);
    bool checkStatus();
    void updateUniformLocations();
};
 
class GlShaders
{
private:
    // shaders
    GLuint vs_debug{};
    GLuint fs_debug{};
    GLuint vs_blit{};
    GLuint fs_blit{};
public:
    // programs
    GlProgram debug{};
    GlProgram blit{};
private:
    GLuint createShader(GLenum type, const char* source);
    void releaseShader(GLuint& shader);
public:
    void initialize(GlContext& context);
    void release(GlContext& context);
};
 
 #endif // _TVG_GL_SHADERS_H_
 