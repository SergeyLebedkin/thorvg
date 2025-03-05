/*
 * Copyright (c) 2024 - 2025 the ThorVG project. All rights reserved.

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

#include "tvgGlCompositor.h"

void GlCompositor::initialize(GlContext& context, GlRenderDataStagedBuffer& stagedBuffer, uint32_t width, uint32_t height)
{
    // shaders
    shaders.initialize(context);
    // composition and blend geometries
    meshData.blitBox(context);
    // store handle to staged buffer
    this->stagedBuffer = &stagedBuffer;
    // store render target dimensions
    this->width = width;
    this->height = height;
}


void GlCompositor::release(GlContext& context)
{
    // composition and blend geometries
    meshData.release(context);
    // shaders
    shaders.release(context);
}


void GlCompositor::update(GlRenderDataStagedBuffer& stagedBuffer)
{
    stagedBuffer.append(&meshData);
}


void GlCompositor::beginRenderPass(GlRenderTarget* target, bool clear, const RenderColor clearColor)
{
    assert(target);
    if (target == currentRenderTraget) return;
    // we must to end render pass first
    endRenderPass();
    // start new render pass
    currentRenderTraget = target;
    frameBufferHandle = target->frameBufferMS;
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, target->frameBufferMS));
    // clear content
    if (clear) {
        GL_CHECK(glClearStencil(0));
        GL_CHECK(glClearDepth(1.0f));
        GL_CHECK(glClearColor(clearColor.r / 255.0f, clearColor.g / 255.0f, clearColor.b / 255.0f, clearColor.a / 255.0f));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    }
    // we must to submit gpu buffers after each render pass start
    assert(stagedBuffer);
    stagedBuffer->bind();
}


void GlCompositor::endRenderPass()
{
    if (currentRenderTraget) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        currentRenderTraget = nullptr;
        frameBufferHandle = 0;
    }
}


void GlCompositor::blit(GlRenderTarget* src)
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);
};