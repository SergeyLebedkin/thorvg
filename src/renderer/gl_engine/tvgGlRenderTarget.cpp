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

#include "tvgGlRenderTarget.h"

void GlRenderTarget::initialize(GlContext& context, uint32_t width, uint32_t height)
{
    this->width = width;
    this->height = height;
    this->sampler = context.samplerNearestRepeat;

    // multisampled color buffer
    GL_CHECK(glGenRenderbuffers(1, &bufferColorMS));
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, bufferColorMS));
    GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, context.preferredFormat, width, height));
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
    // multisampled depth-stencil
    GL_CHECK(glGenRenderbuffers(1, &bufferDepthStencilMS));
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, bufferDepthStencilMS));
    GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height));
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
    // multisampled frame buffer
    GL_CHECK(glGenFramebuffers(1, &frameBufferMS));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferMS));
    GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, bufferColorMS));
    GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, bufferDepthStencilMS));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    // resolved texure
    GL_CHECK(glGenTextures(1, &texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, context.preferredFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
    // resolved frame buffer
    GL_CHECK(glGenFramebuffers(1, &frameBuffer));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}


void GlRenderTarget::release(GlContext& context)
{
    GL_CHECK(glDeleteTextures(1, &texture));
    GL_CHECK(glDeleteRenderbuffers(1, &bufferColorMS));
    GL_CHECK(glDeleteRenderbuffers(1, &bufferDepthStencilMS));
    GL_CHECK(glDeleteFramebuffers(1, &frameBuffer));
    GL_CHECK(glDeleteFramebuffers(1, &frameBufferMS));
    height = 0;
    width = 0;
}


void GlRenderTarget::resolve(GlContext& context)
{
    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBufferMS));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer));
    GL_CHECK(glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST));
    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
}

//*****************************************************************************
// render target pool
//*****************************************************************************

GlRenderTarget* GlRenderTargetPool::allocate(GlContext& context)
{
   GlRenderTarget* renderTarget{};
    if (pool.count > 0) {
       renderTarget = pool.last();
        pool.pop();
    } else {
       renderTarget = new GlRenderTarget;
       renderTarget->initialize(context, width, height);
       list.push(renderTarget);
    }
    return renderTarget;
};
 
 
void GlRenderTargetPool::free(GlContext& context, GlRenderTarget* renderTarget)
{
    pool.push(renderTarget);
};
 
 
void GlRenderTargetPool::initialize(GlContext& context, uint32_t width, uint32_t height)
{
    this->width = width;
    this->height = height;
}

void GlRenderTargetPool::release(GlContext& context)
{
    ARRAY_FOREACH(p, list) {
       (*p)->release(context);
       delete(*p);
    }
    list.clear();
    pool.clear();
    height = 0;
    width = 0;
};
 