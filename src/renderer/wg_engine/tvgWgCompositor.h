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

    // shapes
    void drawShape(WgContext& context, WgRenderDataShape* renderData);
    void blendShape(WgContext& context, WgRenderDataShape* renderData, BlendMethod blendMethod){};
    void clipShape(WgContext& context, WgRenderDataShape* renderData){};

    // strokes
    void drawStrokes(WgContext& context, WgRenderDataShape* renderData){};
    void blendStrokes(WgContext& context, WgRenderDataShape* renderData, BlendMethod blendMethod){};
    void clipStrokes(WgContext& context, WgRenderDataShape* renderData){};

    // images
    void drawImage(WgContext& context, WgRenderDataPicture* renderData){};
    void blendImage(WgContext& context, WgRenderDataPicture* renderData, BlendMethod blendMethod){};
    void clipImage(WgContext& context, WgRenderDataPicture* renderData){};

    // scenes
    void drawScene(WgContext& context, WgRenderTarget* scene, WgCompose* compose){};
    void blendScene(WgContext& context, WgRenderTarget* src, WgCompose* compose){};

    // the renderer prioritizes clipping with the stroke over the shape's fill
    void markupClipPath(WgContext& context, WgRenderDataShape* renderData){};
    void renderClipPath(WgContext& context, WgRenderDataPaint* paint){};
    void clearClipPath(WgContext& context, WgRenderDataPaint* paint){};
public:
    void initialize(WgContext& context, WgRenderDataStagedBuffer& stagedBuffer, uint32_t width, uint32_t height);
    void release(WgContext& context);
    void update(WgRenderDataStagedBuffer& stagedBuffer);

    void beginRenderPass(WGPUCommandEncoder encoder, WgRenderTarget* target, bool clear, const RenderColor clearColor);
    void endRenderPass();

    // render shapes, images and scenes
    void renderShape(WgContext& context, WgRenderDataShape* renderData, BlendMethod blendMethod);
    void renderImage(WgContext& context, WgRenderDataPicture* renderData, BlendMethod blendMethod){};
    void renderScene(WgContext& context, WgRenderTarget* scene, WgCompose* compose){};
    void composeScene(WgContext& context, WgRenderTarget* src, WgRenderTarget* mask, WgCompose* compose){};

    // blit render storage to texture view (f.e. screen buffer)
    void blit(WGPUCommandEncoder encoder, WgRenderTarget* src, WGPUTextureView dstView);
};

#endif // _TVG_WG_COMPOSITOR_H_
