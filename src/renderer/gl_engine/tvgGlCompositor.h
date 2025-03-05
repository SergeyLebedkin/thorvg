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

#ifndef _TVG_GL_COMPOSITOR_H_
#define _TVG_GL_COMPOSITOR_H_

#include "tvgGlRenderTarget.h"
#include "tvgGlRenderData.h"
#include "tvgGlShaders.h"

struct GlCompose: RenderCompositor
{
    BlendMethod blend{};
    RenderRegion aabb{};
};

class GlCompositor 
{
private:
    // shaders
    GlShaders shaders{};
    // composition and blend geometries
    GlMeshData meshData;
    // render target dimensions
    uint32_t width{};
    uint32_t height{};
    // current render pass handles
    GlRenderDataStagedBuffer* stagedBuffer{};
    GLuint frameBufferHandle{};
    GlRenderTarget* currentRenderTraget{}; // external handle

    // shapes
    void drawShape(GlContext& context, GlRenderDataShape* renderData);
    void blendShape(GlContext& context, GlRenderDataShape* renderData, BlendMethod blendMethod){};
    void clipShape(GlContext& context, GlRenderDataShape* renderData){};

    // strokes
    void drawStrokes(GlContext& context, GlRenderDataShape* renderData){};
    void blendStrokes(GlContext& context, GlRenderDataShape* renderData, BlendMethod blendMethod){};
    void clipStrokes(GlContext& context, GlRenderDataShape* renderData){};

    // images
    void drawImage(GlContext& context, GlRenderDataPicture* renderData){};
    void blendImage(GlContext& context, GlRenderDataPicture* renderData, BlendMethod blendMethod){};
    void clipImage(GlContext& context, GlRenderDataPicture* renderData){};

    // scenes
    void drawScene(GlContext& context, GlRenderTarget* scene, GlCompose* compose){};
    void blendScene(GlContext& context, GlRenderTarget* src, GlCompose* compose){};

    // the renderer prioritizes clipping with the stroke over the shape's fill
    void markupClipPath(GlContext& context, GlRenderDataShape* renderData){};
    void renderClipPath(GlContext& context, GlRenderDataPaint* paint){};
    void clearClipPath(GlContext& context, GlRenderDataPaint* paint){};
public:
    void initialize(GlContext& context, GlRenderDataStagedBuffer& stagedBuffer, uint32_t width, uint32_t height);
    void release(GlContext& context);
    void update(GlRenderDataStagedBuffer& stagedBuffer);

    void beginRenderPass(GlRenderTarget* target, bool clear, const RenderColor clearColor);
    void endRenderPass();

    // render shapes, images and scenes
    void renderShape(GlContext& context, GlRenderDataShape* renderData, BlendMethod blendMethod);
    void renderImage(GlContext& context, GlRenderDataPicture* renderData, BlendMethod blendMethod){};
    void renderScene(GlContext& context, GlRenderTarget* scene, GlCompose* compose){};
    void composeScene(GlContext& context, GlRenderTarget* src, GlRenderTarget* mask, GlCompose* compose){};

    // blit render storage to screen
    void blit(GlRenderTarget* src, GLuint dstFramebuffer);
};

#endif // _TVG_GL_COMPOSITOR_H_
