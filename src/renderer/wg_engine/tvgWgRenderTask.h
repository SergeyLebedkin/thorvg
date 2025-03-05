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

 #ifndef _TVG_WG_RENDER_TASK_H_
 #define _TVG_WG_RENDER_TASK_H_
 
 #include "tvgWgCompositor.h"

// base class for any renderable objects 
struct WgRenderTask {
    virtual ~WgRenderTask() {}
    // return true, if task have it own render target
    virtual bool isRenderTarget() { return false; };
    // render tree recurtion depth
    virtual void execute(WgContext& context, WgCompositor& compositor, WGPUCommandEncoder encoder) = 0;
};

// task for sinWge shape rendering
struct WgPaintTask: public WgRenderTask {
    // shape render properties
    WgRenderDataPaint* renderData{};
    BlendMethod blendMethod{};

    WgPaintTask(WgRenderDataPaint* renderData, BlendMethod blendMethod) : 
        renderData(renderData), blendMethod(blendMethod) {}
    // return true, if task have it own render target
    bool isRenderTarget() override { return false; };
    // apply shape execution, including custom blending and clipping
    void execute(WgContext& context, WgCompositor& compositor, WGPUCommandEncoder encoder) override;
};

// task for scene rendering with blending, composition and effect
struct WgSceneTask: public WgRenderTask {
    // parent scene (nullptr for root scene)
    WgSceneTask* parent{};
    // childs can be shapes or scenes tesks
    Array<WgRenderTask*> childs;
    // scene blend/compose targets
    WgRenderTarget* renderTarget{};
    WgRenderTarget* renderTargetMsk{};
    // scene blend/compose properties
    WgCompose* compose{};

    WgSceneTask(WgRenderTarget* renderTarget, WgCompose* compose, WgSceneTask* parent) :
        parent(parent), renderTarget(renderTarget), compose(compose) {}
    // return true, if task have it own render target
    bool isRenderTarget() override { return true; };
    // apply scene execution, including all shapes drawing, blending, composition and effect
    void execute(WgContext& context, WgCompositor& compositor, WGPUCommandEncoder encoder) override;
};
 
 #endif // _TVG_WG_RENDER_TASK_H_
 