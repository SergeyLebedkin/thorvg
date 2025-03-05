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

#include "tvgGlShaderSrc.h"
#include <iostream>

const char* cShaderSrc_vs_Debug = R"(
// attributes
layout(location = 0) in vec2 aPosition;

// uniforms
uniform mat4 uViewMat;
uniform mat4 uModelMat;

// varying
out vec2 vPosition;

void main() {
    gl_Position = uViewMat * uModelMat * vec4(aPosition, 0.0, 1.0);
    vPosition = aPosition;
}
)";

const char* cShaderSrc_fs_Debug = R"(
// varying
in vec2 vPosition;

// uniforms
uniform vec4 uColorDebug;

// output buffers
out vec4 gColor;

void main() {
    gColor = vec4(vPosition, 0.0, 1.0) + uColorDebug;
}
)";

const char* cShaderSrc_vs_Blit = R"(
// attributes
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

// varying
out vec2 vTexCoords;

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vTexCoords = aTexCoord;
}
)";

const char* cShaderSrc_fs_Blit = R"(
// varying
in vec2 vTexCoords;

// uniforms
uniform sampler2D uImageSrc;

// output buffers
out vec4 gColor;

void main() {
    gColor = texture(uImageSrc, vTexCoords.xy);
}
)";