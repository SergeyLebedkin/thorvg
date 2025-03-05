/*
 * Copyright (c) 2024 the ThorVG project. All rights reserved.

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

#ifndef _TVG_WG_COMPOSITOR_H_
#define _TVG_WG_COMPOSITOR_H_

#include "tvgWgRenderTarget.h"
#include "tvgWgRenderData.h"
#include "tvgWgPipelines.h"

struct WgCompose: RenderCompositor
{
    BlendMethod blend{};
    RenderRegion aabb{};
};

class WgCompositor
{
private:
    // pipelines
    WgPipelines pipelines{};
    // global stencil/depth buffer handles
    WGPUTexture texDepthStencil{};
    WGPUTextureView texViewDepthStencil{};
    WGPUTexture texDepthStencilMS{};
    WGPUTextureView texViewDepthStencilMS{};
    // composition and blend geometries
    WgMeshData meshData;
    // render target dimensions
    uint32_t width{};
    uint32_t height{};
    // current render pass handles
    WgRenderDataStagedBuffer* stagedBuffer{};
    WGPURenderPassEncoder renderPassEncoder{};
    WgRenderTarget* currentRenderTraget{}; // external handle
public:
    void initialize(WgContext& context, WgRenderDataStagedBuffer& stagedBuffer, uint32_t width, uint32_t height);
    void release(WgContext& context);
    void update(WgRenderDataStagedBuffer& stagedBuffer);

    void beginRenderPass(WGPUCommandEncoder encoder, WgRenderTarget* target, bool clear, const RenderColor clearColor);
    void endRenderPass();

    // blit render storage to texture view (f.e. screen buffer)
    void blit(WGPUCommandEncoder encoder, WgRenderTarget* src, WGPUTextureView dstView);
};

#endif // _TVG_WG_COMPOSITOR_H_
