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

#include "tvgWgRenderTask.h"

//***********************************************************************
// WgPaintTask
//***********************************************************************

void WgPaintTask::execute(WgContext& context, WgCompositor& compositor, WGPUCommandEncoder encoder)
{
    // if (renderData->type() == tvg::Type::Shape)
    //     std::cout << "Shape: " << (uint32_t)blendMethod << std::endl;
    // if (renderData->type() == tvg::Type::Picture)
    //     std::cout << "Image: " << (uint32_t)blendMethod << std::endl;
    // else assert(true);
}

//***********************************************************************
// GlSceneTask
//***********************************************************************

void WgSceneTask::execute(WgContext& context, WgCompositor& compositor, WGPUCommandEncoder encoder)
{
    //std::cout << "Scene => " << "childs: " << (uint32_t)childs.count << std::endl;
    ARRAY_FOREACH(task, childs) {
        WgRenderTask* renderTask = *task;
        // we can do not start new render pass in a case, if the child have its own
        if (!renderTask->isRenderTarget())
            compositor.beginRenderPass(encoder, renderTarget, false, {0, 0, 0, 0});
        // execute child (shape or scene)
        renderTask->execute(context, compositor, encoder);
    }
    // we must finish current render target for current scene
    compositor.endRenderPass();
    //std::cout << "Scene <= " << std::endl;
}
