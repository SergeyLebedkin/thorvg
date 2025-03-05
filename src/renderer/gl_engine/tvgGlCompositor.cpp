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
}


void GlCompositor::endRenderPass()
{
    if (currentRenderTraget) {
        // we must to submit gpu buffers after each render pass start
        assert(stagedBuffer);
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        currentRenderTraget = nullptr;
        frameBufferHandle = 0;
    }
}


void GlCompositor::renderShape(GlContext& context, GlRenderDataShape* renderData, BlendMethod blendMethod)
{
    assert(renderData);
    assert(currentRenderTraget);
    // apply clip path if neccessary
    if (renderData->clips.count != 0) {
        renderClipPath(context, renderData);
        if (renderData->strokeFirst) {
            clipStrokes(context, renderData);
            clipShape(context, renderData);
        } else {
            clipShape(context, renderData);
            clipStrokes(context, renderData);
        }
        clearClipPath(context, renderData);
    // use custom blending
    } else if (blendMethod != BlendMethod::Normal) {
        if (renderData->strokeFirst) {
            blendStrokes(context, renderData, blendMethod);
            blendShape(context, renderData, blendMethod);
        } else {
            blendShape(context, renderData, blendMethod);
            blendStrokes(context, renderData, blendMethod);
        }
    // use direct hardware blending
    } else {
        if (renderData->strokeFirst) {
            drawStrokes(context, renderData);
            drawShape(context, renderData);
        } else {
            drawShape(context, renderData);
            drawStrokes(context, renderData);
        }
    }
}


void GlCompositor::drawShape(GlContext& context, GlRenderDataShape* renderData)
{
    assert(renderData);
    assert(currentRenderTraget);
    assert(renderData->meshGroupShapes.meshes.count == renderData->meshGroupShapesBBox.meshes.count);
    if (renderData->renderSettingsShape.skip) return;
    if (renderData->meshGroupShapes.meshes.count == 0) return;
    if ((renderData->viewport.w <= 0) || (renderData->viewport.h <= 0)) return;
    glScissor(renderData->viewport.x, renderData->viewport.y, renderData->viewport.w, renderData->viewport.h);
}


void GlCompositor::blit(GlRenderTarget* src, GLuint dstFramebuffer)
{
    // bind current render target
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, dstFramebuffer));
    GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
    
    // apply viewport
    GL_CHECK(glViewport(0, 0, width, height));
    GL_CHECK(glScissor(0, 0, width, height));

    // setup pipeline
    GL_CHECK(glUseProgram(shaders.blit.prog));
    GL_CHECK(glUniform1i(shaders.blit.uloc.uImageSrc, 0));
    GL_CHECK(glBindSampler(0, src->sampler));
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, src->texture));

    // draw mesh
    stagedBuffer->bind(meshData.voffset, meshData.toffset);
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glDrawElements(GL_TRIANGLES, meshData.ibuffer.count, GL_UNSIGNED_INT, (const void*)meshData.ioffset));
    GL_CHECK(glDisableVertexAttribArray(1));
    GL_CHECK(glDisableVertexAttribArray(0));

    // clean up
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    GL_CHECK(glUseProgram(0));
    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
};