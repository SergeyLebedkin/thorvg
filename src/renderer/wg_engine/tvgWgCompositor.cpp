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

#include "tvgWgCompositor.h"

void WgCompositor::initialize(WgContext& context, WgRenderDataStagedBuffer& stagedBuffer, uint32_t width, uint32_t height)
{
    // pipelines
    pipelines.initialize(context);
    // allocate global stencil buffer handles
    texDepthStencil = context.createTexAttachement(width, height, WGPUTextureFormat_Depth24PlusStencil8, 1);
    texViewDepthStencil = context.createTextureView(texDepthStencil);
    texDepthStencilMS = context.createTexAttachement(width, height, WGPUTextureFormat_Depth24PlusStencil8, 4);
    texViewDepthStencilMS = context.createTextureView(texDepthStencilMS);
    // composition and blend geometries
    meshData.blitBox(context);
    // store handle to staged buffer
    this->stagedBuffer = &stagedBuffer;
    // store render target dimensions
    this->width = width;
    this->height = height;
}


void WgCompositor::release(WgContext& context)
{
    // release global stencil buffer handles
    context.releaseTextureView(texViewDepthStencilMS);
    context.releaseTexture(texDepthStencilMS);
    context.releaseTextureView(texViewDepthStencil);
    context.releaseTexture(texDepthStencil);
    // composition and blend geometries
    meshData.release(context);
    // pipelines
    pipelines.release(context);
}


void WgCompositor::update(WgRenderDataStagedBuffer& stagedBuffer)
{
    stagedBuffer.append(&meshData);
}


void WgCompositor::beginRenderPass(WGPUCommandEncoder encoder, WgRenderTarget* target, bool clear, const RenderColor clearColor)
{
    assert(target);
    // do not start same render bass
    if (target == currentRenderTraget) return;
    // we must to end render pass first
    endRenderPass();
    // start new render pass
    currentRenderTraget = target;
    const WGPURenderPassDepthStencilAttachment depthStencilAttachment {
        .view = texViewDepthStencilMS,
        .depthLoadOp = WGPULoadOp_Clear,
        .depthStoreOp = WGPUStoreOp_Discard,
        .depthClearValue = 1.0f,
        .stencilLoadOp = WGPULoadOp_Clear,
        .stencilStoreOp = WGPUStoreOp_Discard,
        .stencilClearValue = 0
    };
    const WGPURenderPassColorAttachment colorAttachment {
        .view = target->texViewMS,
        .resolveTarget = target->texView,
        .loadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = {
            clearColor.r / 255.0,
            clearColor.g / 255.0,
            clearColor.b / 255.0,
            clearColor.a / 255.0
        },
        #ifdef __EMSCRIPTEN__
        .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        #endif
    };
    const WGPURenderPassDescriptor renderPassDesc {
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = &depthStencilAttachment
    };
    renderPassEncoder = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    assert(renderPassEncoder);
    // we must to submit gpu buffers after each render pass start
    assert(stagedBuffer);
    stagedBuffer->bind(renderPassEncoder);
}


void WgCompositor::endRenderPass()
{
    if (currentRenderTraget) {
        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
        currentRenderTraget = nullptr;
        renderPassEncoder = nullptr;
    }
}


void WgCompositor::blit(WGPUCommandEncoder encoder, WgRenderTarget* src, WGPUTextureView dstView) {
    const WGPURenderPassColorAttachment colorAttachment {
        .view = dstView,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = { 0.1, 0.1, 0.1, 1.0 },
    };
    const WGPURenderPassDescriptor renderPassDesc{
        .colorAttachmentCount = 1, 
        .colorAttachments = &colorAttachment
    };
    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    wgpuRenderPassEncoderSetViewport(renderPass, 0, 0, width, height, 0, 1);
    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);
}