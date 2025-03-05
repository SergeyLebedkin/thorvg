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

#include <atomic>
#include "tvgTaskScheduler.h"
#include "tvgGlRenderer.h"


/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

static atomic<int32_t> initEngineCnt{};
static atomic<int32_t> rendererCnt{};


static void _termEngine()
{
    if (rendererCnt > 0) return;
}


void GlRenderer::release()
{
    // dispose stored objects
    disposeObjects();

    // clear render data paint pools
    mRenderDataStagedBuffer.release(mContext);
    mRenderDataShapePool.release(mContext);
    mRenderDataPicturePool.release(mContext);
    GlMeshDataPool::gMeshDataPool->release(mContext);

    // clear render storage pool
    mRenderTargetPool.release(mContext);

    // clear render target pool
    mCompositorList.clear();
    mRenderTargetStack.clear();
    mRenderTargetRoot.release(mContext);

    // release context handles
    mCompositor.release(mContext);
    mContext.release();
}


void GlRenderer::disposeObjects()
{
    ARRAY_FOREACH(p, mDisposeRenderDatas) {
        auto renderData = (GlRenderDataPaint*)(*p);
        if (renderData->type() == Type::Shape) {
            mRenderDataShapePool.free(mContext, (GlRenderDataShape*)renderData);
        } else {
            mRenderDataPicturePool.free(mContext, (GlRenderDataPicture*)renderData);
        }
    }
    mDisposeRenderDatas.clear();
}


/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/


RenderData GlRenderer::prepare(const RenderShape& rshape, RenderData data, const Matrix& transform, Array<RenderData>& clips, uint8_t opacity, RenderUpdateFlag flags, bool clipper)
{
    // get or create render data shape
    auto renderDataShape = (GlRenderDataShape*)data;
    if (!renderDataShape)
        renderDataShape = mRenderDataShapePool.allocate(mContext);

    // update geometry
    if ((!data) || (flags & (RenderUpdateFlag::Path | RenderUpdateFlag::Stroke))) {
        renderDataShape->updateMeshes(mContext, rshape, transform, mBufferPool.pool);
    }

    // update paint settings
    if ((!data) || (flags & (RenderUpdateFlag::Transform | RenderUpdateFlag::Blend))) {
        renderDataShape->update(mContext, transform, mTargetSurface.cs, opacity);
        renderDataShape->fillRule = rshape.rule;
    }

    // setup fill settings
    renderDataShape->viewport = mViewport;
    renderDataShape->opacity = opacity;
    if (flags & RenderUpdateFlag::Gradient && rshape.fill) renderDataShape->renderSettingsShape.updateFill(mContext, rshape.fill);
    else if (flags & RenderUpdateFlag::Color) renderDataShape->renderSettingsShape.updateColor(mContext, rshape.color);
    if (rshape.stroke) {
        if (flags & RenderUpdateFlag::GradientStroke && rshape.stroke->fill) renderDataShape->renderSettingsStroke.updateFill(mContext, rshape.stroke->fill);
        else if (flags & RenderUpdateFlag::Stroke) renderDataShape->renderSettingsStroke.updateColor(mContext, rshape.stroke->color);
    }

    // store clips data
    renderDataShape->updateClips(clips);

    return renderDataShape;
}


RenderData GlRenderer::prepare(RenderSurface* surface, RenderData data, const Matrix& transform, Array<RenderData>& clips, uint8_t opacity, RenderUpdateFlag flags)
{
        // get or create render data shape
    auto renderDataPicture = (GlRenderDataPicture*)data;
    if (!renderDataPicture)
        renderDataPicture = mRenderDataPicturePool.allocate(mContext);

    // update paint settings
    renderDataPicture->viewport = mViewport;
    renderDataPicture->opacity = opacity;
    if (flags & (RenderUpdateFlag::Transform | RenderUpdateFlag::Blend)) {
        renderDataPicture->update(mContext, transform, surface->cs, opacity);
    }

    // update image data
    if (flags & (RenderUpdateFlag::Path | RenderUpdateFlag::Image)) {
        renderDataPicture->updateSurface(mContext, surface);
    }

    // store clips data
    renderDataPicture->updateClips(clips);

    return renderDataPicture;
}

bool GlRenderer::preRender()
{
    mRenderDataStagedBuffer.clear();
    // push root render storage to the render tree stack
    assert(mRenderTargetStack.count == 0);
    mRenderTargetStack.push(&mRenderTargetRoot);
    // create root compose settings
    GlCompose* compose = new GlCompose();
    compose->aabb = {0, 0, (int32_t)mTargetSurface.w, (int32_t)mTargetSurface.h };
    compose->blend = BlendMethod::Normal;
    compose->method = MaskMethod::None;
    compose->opacity = 255;
    mCompositorList.push(compose);
    // create root scene scene
    GlSceneTask* sceneTask = new GlSceneTask(&mRenderTargetRoot, compose, nullptr);
    mRenderTaskList.push(sceneTask);
    mSceneTaskStack.push(sceneTask);
    // update static staged data
    mCompositor.update(mRenderDataStagedBuffer);
    return true;
}


bool GlRenderer::renderShape(RenderData data)
{
    // create new paint task
    GlPaintTask* paintTask = new GlPaintTask((GlRenderDataPaint*)data, mBlendMethod);
    // append shape task to current scene task
    GlSceneTask* sceneTask = mSceneTaskStack.last();
    sceneTask->childs.push(paintTask);
    // append shape task to tasks list
    mRenderTaskList.push(paintTask);
    // append data to staged buffer
    mRenderDataStagedBuffer.append((GlRenderDataShape*)data);
    return true;
}


bool GlRenderer::renderImage(RenderData data)
{
    // create new paint task
    GlPaintTask* paintTask = new GlPaintTask((GlRenderDataPaint*)data, mBlendMethod);
    // append shape task to current scene task
    GlSceneTask* sceneTask = mSceneTaskStack.last();
    sceneTask->childs.push(paintTask);
    // append shape task to tasks list
    mRenderTaskList.push(paintTask);
    // append data to staged buffer
    mRenderDataStagedBuffer.append((GlRenderDataPicture*)data);
    return true;
}


bool GlRenderer::postRender()
{
    mRenderDataStagedBuffer.flush(mContext);
    // execure rendering (all the fun is here)
    GlSceneTask* sceneTaskRoot = mSceneTaskStack.last();
    sceneTaskRoot->execute(mContext, mCompositor);
    // pop root scene task
    mSceneTaskStack.pop();
    assert(mSceneTaskStack.count == 0);
    // pop root render storage from the render tree stack
    mRenderTargetStack.pop();
    assert(mRenderTargetStack.count == 0);
    // delete all tasks
    ARRAY_FOREACH(p, mRenderTaskList) { delete (*p); };
    mRenderTaskList.clear();
    // delete all compositions
    ARRAY_FOREACH(p, mCompositorList) { delete (*p); };
    mCompositorList.clear();
    return true;
}


void GlRenderer::dispose(RenderData data) {
    ScopedLock lock(mDisposeKey);
    mDisposeRenderDatas.push(data);
}


RenderRegion GlRenderer::region(RenderData data)
{
    auto renderData = (GlRenderDataPaint*)data;
    if (renderData->type() == Type::Shape) {
        auto& v1 = renderData->aabb.min;
        auto& v2 = renderData->aabb.max;
        RenderRegion renderRegion;
        renderRegion.x = static_cast<int32_t>(nearbyint(v1.x));
        renderRegion.y = static_cast<int32_t>(nearbyint(v1.y));
        renderRegion.w = static_cast<int32_t>(nearbyint(v2.x)) - renderRegion.x;
        renderRegion.h = static_cast<int32_t>(nearbyint(v2.y)) - renderRegion.y;
        return renderRegion;
    }
    return { 0, 0, (int32_t)mTargetSurface.w, (int32_t)mTargetSurface.h };
}


RenderRegion GlRenderer::viewport() {
    return mViewport;
}


bool GlRenderer::viewport(const RenderRegion& vp)
{
    mViewport = vp;
    return true;
}


bool GlRenderer::blend(BlendMethod method)
{
    mBlendMethod = method;
    return true;
}


ColorSpace GlRenderer::colorSpace()
{
    return ColorSpace::Unknown;
}


const RenderSurface* GlRenderer::mainSurface()
{
    return &mTargetSurface;
}


bool GlRenderer::clear()
{
    //TODO: clear the current target buffer only if clear() is called
    return true;
}


bool GlRenderer::sync()
{
    disposeObjects();
    // show root offscreen buffer
    mCompositor.blit(&mRenderTargetRoot);
    return true;
}

bool GlRenderer::target(void* context, int32_t id, uint32_t width, uint32_t height)
{
    // update render targets dimentions
    if ((mTargetSurface.w != width) || (mTargetSurface.h != height)) {
        // release all handles
        release();

        // initialize base rendering handles
        mContext.initialize();

        // initialize render tree instances
        mRenderTargetPool.initialize(mContext, width, height);
        mRenderTargetRoot.initialize(mContext, width, height);
        mCompositor.initialize(mContext, mRenderDataStagedBuffer, width, height);

        // store target properties
        mTargetSurface.stride = width;
        mTargetSurface.w = width;
        mTargetSurface.h = height;
        return true;
    }

    return true;
}


GlRenderer::GlRenderer()
{
    if (TaskScheduler::onthread()) {
        TVGLOG("WG_RENDERER", "Running on a non-dominant thread!, Renderer(%p)", this);
        mBufferPool.pool = new GlGeometryBufferPool;
        mBufferPool.individual = true;
    } else {
        mBufferPool.pool = GlGeometryBufferPool::instance();
    }
}


GlRenderer::~GlRenderer()
{
    release();

    if (mBufferPool.individual) delete(mBufferPool.pool);

    --rendererCnt;

    if (rendererCnt == 0 && initEngineCnt == 0) _termEngine();
}


RenderCompositor* GlRenderer::target(const RenderRegion& region, TVG_UNUSED ColorSpace cs, TVG_UNUSED CompositionFlag flags)
{
    // create and setup compose data
    GlCompose* compose = new GlCompose();
    compose->aabb = region;
    mCompositorList.push(compose);
    return compose;
}


bool GlRenderer::beginComposite(RenderCompositor* cmp, MaskMethod method, uint8_t opacity)
{
    // save current composition settings
    GlCompose* compose = (GlCompose*)cmp;
    compose->method = method;
    compose->opacity = opacity;
    compose->blend = mBlendMethod;
    GlSceneTask* sceneTaskCurrent = mSceneTaskStack.last();
    // allocate new render storage and push to the render tree stack
    GlRenderTarget* renderTarget = mRenderTargetPool.allocate(mContext);
    mRenderTargetStack.push(renderTarget);
    // create and setup new scene task
    GlSceneTask* sceneTask = new GlSceneTask(renderTarget, compose, sceneTaskCurrent);
    sceneTaskCurrent->childs.push(sceneTask);
    mRenderTaskList.push(sceneTask);
    mSceneTaskStack.push(sceneTask);
    return true;
}


bool GlRenderer::endComposite(RenderCompositor* cmp)
{
    // finish scene blending
    if (cmp->method == MaskMethod::None) {
        // get source and destination render targets
        GlRenderTarget* src = mRenderTargetStack.last();
        mRenderTargetStack.pop();
        // pop source scene
        mSceneTaskStack.pop();
        // back render targets to the pool
        mRenderTargetPool.free(mContext, src);
    } else { // finish scene composition
        // get source, mask and destination render targets
        GlRenderTarget* src = mRenderTargetStack.last();
        mRenderTargetStack.pop();
        GlRenderTarget* msk = mRenderTargetStack.last();
        mRenderTargetStack.pop();
        // get source and mask scenes
        GlSceneTask* srcScene = mSceneTaskStack.last();
        mSceneTaskStack.pop();
        GlSceneTask* mskScene = mSceneTaskStack.last();
        mSceneTaskStack.pop();
        // setup render target compose destitations
        srcScene->renderTargetMsk = mskScene->renderTarget;
        // back render targets to the pool
        mRenderTargetPool.free(mContext, src);
        mRenderTargetPool.free(mContext, msk);
    }
    return true;
}


void GlRenderer::prepare(RenderEffect* effect, const Matrix& transform)
{
}


bool GlRenderer::region(RenderEffect* effect)
{
    return false;
}


bool GlRenderer::render(RenderCompositor* cmp, const RenderEffect* effect, TVG_UNUSED bool direct)
{
    return false;
}


void GlRenderer::dispose(RenderEffect* effect)
{
};


bool GlRenderer::preUpdate()
{
    return true;
}


bool GlRenderer::postUpdate()
{
    return true;
}


bool GlRenderer::init(TVG_UNUSED uint32_t threads)
{
    if ((initEngineCnt++) > 0) return true;

    //TODO: global engine init

    return true;
}


int32_t GlRenderer::init()
{
    return initEngineCnt;
}


bool GlRenderer::term()
{
    if ((--initEngineCnt) > 0) return true;

    initEngineCnt = 0;

    _termEngine();

    return true;
}


GlRenderer* GlRenderer::gen()
{
    ++rendererCnt;
    return new GlRenderer();
}
