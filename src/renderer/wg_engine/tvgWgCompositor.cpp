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


void WgCompositor::renderShape(WgContext& context, WgRenderDataShape* renderData, BlendMethod blendMethod)
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


void WgCompositor::drawShape(WgContext& context, WgRenderDataShape* renderData)
{
    assert(renderData);
    assert(currentRenderTraget);
    assert(renderData->meshGroupShapes.meshes.count == renderData->meshGroupShapesBBox.meshes.count);
    if (renderData->renderSettingsShape.skip) return;
    if (renderData->meshGroupShapes.meshes.count == 0) return;
    if ((renderData->viewport.w <= 0) || (renderData->viewport.h <= 0)) return;
    wgpuRenderPassEncoderSetScissorRect(renderPassEncoder, renderData->viewport.x, renderData->viewport.y, renderData->viewport.w, renderData->viewport.h);
    // // setup stencil rules
    // WGPURenderPipeline stencilPipeline = (renderData->fillRule == FillRule::NonZero) ? pipelines.nonzero : pipelines.evenodd;
    // wgpuRenderPassEncoderSetStencilReference(renderPassEncoder, 0);
    // wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0, bindGroupViewMat, 0, nullptr);
    // wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 1, renderData->bindGroupPaint, 0, nullptr);
    // wgpuRenderPassEncoderSetPipeline(renderPassEncoder, stencilPipeline);
    // // draw to stencil (first pass)
    // ARRAY_FOREACH(p, renderData->meshGroupShapes.meshes)
    //     (*p)->drawFan(context, renderPassEncoder);
    // // setup fill rules
    // wgpuRenderPassEncoderSetStencilReference(renderPassEncoder, 0);
    // wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0, bindGroupViewMat, 0, nullptr);
    // wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 1, renderData->bindGroupPaint, 0, nullptr);
    // WgRenderSettings& settings = renderData->renderSettingsShape;
    // if (settings.fillType == WgRenderSettingsType::Solid) {
    //     wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 2, settings.bindGroupSolid, 0, nullptr);
    //     wgpuRenderPassEncoderSetPipeline(renderPassEncoder, pipelines.solid);
    // } else if (settings.fillType == WgRenderSettingsType::Linear) {
    //     wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 2, settings.bindGroupGradient, 0, nullptr);
    //     wgpuRenderPassEncoderSetPipeline(renderPassEncoder, pipelines.linear);
    // } else if (settings.fillType == WgRenderSettingsType::Radial) {
    //     wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 2, settings.bindGroupGradient, 0, nullptr);
    //     wgpuRenderPassEncoderSetPipeline(renderPassEncoder, pipelines.radial);
    // }
    // // draw to color (second pass)
    // renderData->meshDataBBox.drawFan(context, renderPassEncoder);
}


void WgCompositor::blit(WGPUCommandEncoder encoder, WgRenderTarget* src, WGPUTextureView dstView) {
    // bind current render target
    const WGPURenderPassDepthStencilAttachment depthStencilAttachment{ 
        .view = texViewDepthStencil,
        .depthLoadOp = WGPULoadOp_Load,
        .depthStoreOp = WGPUStoreOp_Discard,
        .stencilLoadOp = WGPULoadOp_Load,
        .stencilStoreOp = WGPUStoreOp_Discard
    };
    const WGPURenderPassColorAttachment colorAttachment {
        .view = dstView,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = { 0.0, 0.0, 0.0, 1.0 },
        #ifdef __EMSCRIPTEN__
        .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED
        #endif
    };
    const WGPURenderPassDescriptor renderPassDesc{
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = &depthStencilAttachment
    };
    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    // apply viewport
    wgpuRenderPassEncoderSetViewport(renderPass, 0, 0, width, height, 0, 1);
    wgpuRenderPassEncoderSetScissorRect(renderPass, 0, 0, width, height);

    // setup pipeline
    wgpuRenderPassEncoderSetBindGroup(renderPass, 0, src->bindGroupTexure, 0, nullptr);
    wgpuRenderPassEncoderSetPipeline(renderPass, pipelines.blit);

    // draw mesh
    stagedBuffer->bind(renderPass, meshData.voffset, meshData.toffset);
    wgpuRenderPassEncoderDrawIndexed(renderPass, meshData.ibuffer.count, 1, meshData.ioffset / 4, 0, 0);

    // clean up
    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);
}