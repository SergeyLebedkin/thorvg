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

#include "tvgGlShaders.h"
#include "tvgGlShaderSrc.h"
#include <iostream>
#include <cassert>

void GlProgram::initialize(GlContext& context, GLuint vertexShader, GLuint fragmentShader)
{
    release(context);
    // create and link program
    prog = glCreateProgram();
    GL_CHECK(glAttachShader(prog, vertexShader));
    GL_CHECK(glAttachShader(prog, fragmentShader));
    GL_CHECK(glLinkProgram(prog));

    // update uniform locations
    if (checkStatus())
        updateUniformLocations();
}


void GlProgram::release(GlContext& context)
{
    glDeleteProgram(prog);
    prog = 0;
}


bool GlProgram::checkStatus()
{
    // check errors
    GLint status{};
    GL_CHECK(glGetProgramiv(prog, GL_LINK_STATUS, &status));
    if (!status) {
        GLsizei errorLength{};
        GLchar errorLog[8192]{};
        GL_CHECK(glGetProgramInfoLog(prog, sizeof(errorLog), &errorLength, errorLog));
        std::cerr << errorLog << std::endl;
        GL_CHECK(glDeleteProgram(prog));
        prog = 0;
        return false;
    }
    return true;
}


void GlProgram::updateUniformLocations()
{
    // uniforms
    uloc.uViewMat = glGetUniformLocation(prog, "uViewMat");
    uloc.uModelMat = glGetUniformLocation(prog, "uModelMat");
    uloc.uColorDebug = glGetUniformLocation(prog, "uColorDebug");
    // textures
    uloc.uImageSrc = glGetUniformLocation(prog, "uImageSrc");
}


GLuint GlShaders::createShader(GLenum type, const char* source)
{
    const char* shaderPack[3];
    // but in general All Desktop GPU should use OpenGL version ( #version 330 core )
    #if defined (THORVG_GL_TARGET_GLES)
    shaderPack[0] ="#version 300 es\n";
    #else
    shaderPack[0] ="#version 330 core\n";
    #endif
    shaderPack[1] = "precision highp float;\n precision highp int;\n";
    shaderPack[2] = source;

    // create and compile shader
    GLuint shader = glCreateShader(type);
    GL_CHECK(glShaderSource(shader, 3, shaderPack, nullptr));
    GL_CHECK(glCompileShader(shader));

    // check errors
    GLint status{};
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (!status) {
        GLsizei errorLength{};
        GLchar errorLog[512]{};
        GL_CHECK(glGetShaderInfoLog(shader, sizeof(errorLog), &errorLength, errorLog));
        std::cerr << errorLog << std::endl;
        GL_CHECK(glDeleteShader(shader));
        return 0;
    }
    return shader;
}


void GlShaders::releaseShader(GLuint& shader)
{
    if (shader) {
        glDeleteShader(shader);
        shader = 0;
    }
}


void GlShaders::initialize(GlContext& context)
{
    // shaders
    vs_debug = createShader(GL_VERTEX_SHADER, cShaderSrc_vs_Debug);
    fs_debug = createShader(GL_FRAGMENT_SHADER, cShaderSrc_fs_Debug);
    vs_blit = createShader(GL_VERTEX_SHADER, cShaderSrc_vs_Blit);
    fs_blit = createShader(GL_FRAGMENT_SHADER, cShaderSrc_fs_Blit);
    // programs
    debug.initialize(context, vs_debug, fs_debug);
    blit.initialize(context, vs_blit, fs_blit);
}


void GlShaders::release(GlContext& context)
{
    // programs
    blit.release(context);
    debug.release(context);
    // shaders
    releaseShader(vs_blit);
    releaseShader(fs_blit);
    releaseShader(vs_debug);
    releaseShader(fs_debug);
}
