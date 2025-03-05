
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

#include "tvgGlRenderData.h"
#include <algorithm>

//***********************************************************************
// GlMeshData
//***********************************************************************

void GlMeshData::update(GlContext& context, const GlVertexBuffer& vertexBuffer)
{
    assert(vertexBuffer.count > 2);
    // setup vertex data
    vbuffer.reserve(vertexBuffer.count);
    vbuffer.count = vertexBuffer.count;
    memcpy(vbuffer.data, vertexBuffer.data, sizeof(vertexBuffer.data[0])*vertexBuffer.count);
    // setup tex coords data
    tbuffer.clear();
    // setup index data
    ibuffer.clear();
}


void GlMeshData::update(GlContext& context, const GlIndexedVertexBuffer& vertexBufferInd)
{
    assert(vertexBufferInd.vcount > 2);
    // setup vertex data
    vbuffer.reserve(vertexBufferInd.vcount);
    vbuffer.count = vertexBufferInd.vcount;
    memcpy(vbuffer.data, vertexBufferInd.vbuff, sizeof(vertexBufferInd.vbuff[0])*vertexBufferInd.vcount);
    // setup tex coords data
    tbuffer.clear();
    // copy index data
    ibuffer.reserve(vertexBufferInd.icount);
    ibuffer.count = vertexBufferInd.icount;
    memcpy(ibuffer.data, vertexBufferInd.ibuff, sizeof(vertexBufferInd.ibuff[0])*vertexBufferInd.icount);
};


void GlMeshData::bbox(GlContext& context, const Point pmin, const Point pmax)
{
    const float data[] = {pmin.x, pmin.y, pmax.x, pmin.y, pmax.x, pmax.y, pmin.x, pmax.y};
    // setup vertex data
    vbuffer.reserve(4);
    vbuffer.count = 4;
    memcpy(vbuffer.data, data, sizeof(data));
    // setup tex coords data
    tbuffer.clear();
    // setup indexes data
    ibuffer.clear();
}


void GlMeshData::imageBox(GlContext& context, float w, float h)
{
    const float vdata[] = {0.0f, 0.0f, w, 0.0f, w, h, 0.0f, h};
    const float tdata[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    const uint32_t idata[] = {0, 1, 2, 0, 2, 3};
    // setup vertex data
    vbuffer.reserve(4);
    vbuffer.count = 4;
    memcpy(vbuffer.data, vdata, sizeof(vdata));
    // setup tex coords data
    tbuffer.reserve(4);
    tbuffer.count = 4;
    memcpy(tbuffer.data, tdata, sizeof(tdata));
    // setup indexes data
    ibuffer.reserve(6);
    ibuffer.count = 6;
    memcpy(ibuffer.data, idata, sizeof(idata));
}


void GlMeshData::blitBox(GlContext& context)
{
    const float vdata[] = {-1.0f, +1.0f, +1.0f, +1.0f, +1.0f, -1.0f, -1.0f, -1.0f};
    const float tdata[] = {+0.0f, +0.0f, +1.0f, +0.0f, +1.0f, +1.0f, +0.0f, +1.0f};
    const uint32_t idata[] = { 0, 1, 2, 0, 2, 3 };
    // setup vertex data
    vbuffer.reserve(4);
    vbuffer.count = 4;
    memcpy(vbuffer.data, vdata, sizeof(vdata));
    // setup tex coords data
    tbuffer.reserve(4);
    tbuffer.count = 4;
    memcpy(tbuffer.data, tdata, sizeof(tdata));
    // setup indexes data
    ibuffer.reserve(6);
    ibuffer.count = 6;
    memcpy(ibuffer.data, idata, sizeof(idata));
}

//***********************************************************************
// GlMeshDataPool
//***********************************************************************

GlMeshData* GlMeshDataPool::allocate(GlContext& context)
{
    GlMeshData* meshData{};
    if (mPool.count > 0) {
        meshData = mPool.last();
        mPool.pop();
    } else {
        meshData = new GlMeshData();
        mList.push(meshData);
    }
    return meshData;
}


void GlMeshDataPool::free(GlContext& context, GlMeshData* meshData)
{
    mPool.push(meshData);
}


void GlMeshDataPool::release(GlContext& context)
{
    ARRAY_FOREACH(p, mList) {
        (*p)->release(context);
        delete(*p);
    }
    mPool.clear();
    mList.clear();
}

GlMeshDataPool gGlMeshDataPoolInstance;
GlMeshDataPool* GlMeshDataPool::gMeshDataPool = &gGlMeshDataPoolInstance;

//***********************************************************************
// GlMeshDataGroup
//***********************************************************************

void GlMeshDataGroup::append(GlContext& context, const GlVertexBuffer& vertexBuffer)
{
    assert(vertexBuffer.count >= 3);
    meshes.push(GlMeshDataPool::gMeshDataPool->allocate(context));
    meshes.last()->update(context, vertexBuffer);
}


void GlMeshDataGroup::append(GlContext& context, const GlIndexedVertexBuffer& vertexBufferInd)
{
    assert(vertexBufferInd.vcount >= 3);
    meshes.push(GlMeshDataPool::gMeshDataPool->allocate(context));
    meshes.last()->update(context, vertexBufferInd);
}


void GlMeshDataGroup::append(GlContext& context, const Point pmin, const Point pmax)
{
    meshes.push(GlMeshDataPool::gMeshDataPool->allocate(context));
    meshes.last()->bbox(context, pmin, pmax);
}


void GlMeshDataGroup::release(GlContext& context)
{
    ARRAY_FOREACH(p, meshes)
        GlMeshDataPool::gMeshDataPool->free(context, *p);
    meshes.clear();
};


//***********************************************************************
// GlImageData
//***********************************************************************

void GlImageData::update(GlContext& context, const RenderSurface* surface)
{
    // get appropriate texture format from color space
    GLint texFormat = GL_BGRA;
    if (surface->cs == ColorSpace::ABGR8888S)
        texFormat = GL_RGBA;
    if (surface->cs == ColorSpace::Grayscale8)
        texFormat = GL_R8;
    // allocate new texture handle
    context.allocateTexture(texture, surface->w, surface->h, texFormat, surface->data);
};


void GlImageData::release(GlContext& context)
{
    context.releaseTexture(texture);
};

//***********************************************************************
// GlRenderSettings
//***********************************************************************

void GlRenderSettings::updateFill(GlContext& context, const Fill* fill)
{
    rasterType = GlRenderRasterType::Gradient;
    // get gradient transfrom matrix
    Matrix invFillTransform;
    if (inverse(&fill->transform(), &invFillTransform))
        gradientTrans.update(invFillTransform);
    // get gradient rasterisation settings
    if (fill->type() == Type::LinearGradient) {
        gradient.update((LinearGradient*)fill);
        fillType = GlRenderSettingsType::Linear;
    } else if (fill->type() == Type::RadialGradient) {
        gradient.update((RadialGradient*)fill);
        fillType = GlRenderSettingsType::Radial;
    }
    // get sampler by spread type and updat texture data
    if ((fill->type() == Type::LinearGradient) || (fill->type() == Type::RadialGradient)) {
        sampler = context.samplerLinearClamp;
        if (fill->spread() == FillSpread::Reflect) sampler = context.samplerLinearMirror;
        if (fill->spread() == FillSpread::Repeat) sampler = context.samplerLinearRepeat;
        context.allocateTexture(texGradient, GL_TEXTURE_GRADIENT_SIZE, 1, GL_RGBA, gradient.texData);
    }
    skip = false;
};


void GlRenderSettings::updateColor(GlContext& context, const RenderColor& c)
{
    rasterType = GlRenderRasterType::Solid;
    solidColor.update(c);
    fillType = GlRenderSettingsType::Solid;
    skip = (c.a == 0);
};


void GlRenderSettings::release(GlContext& context)
{
    context.releaseTexture(texGradient);
};

//***********************************************************************
// GlRenderDataPaint
//***********************************************************************

void GlRenderDataPaint::release(GlContext& context)
{
    clips.clear();
};


void GlRenderDataPaint::update(GlContext& context, const tvg::Matrix& transform, tvg::ColorSpace cs, uint8_t opacity)
{
    modelMat.update(transform);
    blendSettings.update(cs, opacity);
}


void GlRenderDataPaint::updateClips(tvg::Array<tvg::RenderData> &clips) {
    this->clips.clear();
    ARRAY_FOREACH(p, clips) {
        this->clips.push((GlRenderDataPaint*)(*p));
    }
}

//***********************************************************************
// GlRenderDataShape
//***********************************************************************

void GlRenderDataShape::appendShape(GlContext& context, const GlVertexBuffer& vertexBuffer)
{
    if (vertexBuffer.count < 3) return;
    Point pmin{}, pmax{};
    vertexBuffer.getMinMax(pmin, pmax);
    meshGroupShapes.append(context, vertexBuffer);
    meshGroupShapesBBox.append(context, pmin, pmax);
    updateBBox(pmin, pmax);
}


void GlRenderDataShape::appendStroke(GlContext& context, const GlIndexedVertexBuffer& vertexBufferInd)
{
    if (vertexBufferInd.vcount < 3) return;
    Point pmin{}, pmax{};
    vertexBufferInd.getMinMax(pmin, pmax);
    meshGroupStrokes.append(context, vertexBufferInd);
    meshGroupStrokesBBox.append(context, pmin, pmax);
    updateBBox(pmin, pmax);
}


void GlRenderDataShape::updateBBox(Point pmin, Point pmax)
{
    pMin.x = std::min(pMin.x, pmin.x);
    pMin.y = std::min(pMin.y, pmin.y);
    pMax.x = std::max(pMax.x, pmax.x);
    pMax.y = std::max(pMax.y, pmax.y);
}


void GlRenderDataShape::updateAABB(const Matrix& tr) {
    auto p0 = Point{pMin.x, pMin.y} * tr;
    auto p1 = Point{pMax.x, pMin.y} * tr;
    auto p2 = Point{pMin.x, pMax.y} * tr;
    auto p3 = Point{pMax.x, pMax.y} * tr;
    aabb.min = {std::min({p0.x, p1.x, p2.x, p3.x}), std::min({p0.y, p1.y, p2.y, p3.y})};
    aabb.max = {std::max({p0.x, p1.x, p2.x, p3.x}), std::max({p0.y, p1.y, p2.y, p3.y})};
}


void GlRenderDataShape::updateMeshes(GlContext& context, const RenderShape &rshape, const Matrix& tr, GlGeometryBufferPool* pool)
{
    releaseMeshes(context);
    strokeFirst = rshape.stroke ? rshape.stroke->strokeFirst : false;

    // get object scale
    float scale = std::max(std::min(length(Point{tr.e11 + tr.e12,tr.e21 + tr.e22}), 8.0f), 1.0f);

    // path decoded vertex buffer
    auto pbuff = pool->reqVertexBuffer(scale);

    pbuff->decodePath(rshape, true, [&](const GlVertexBuffer& path_buff) {
        appendShape(context, path_buff);
        if ((rshape.stroke) && (rshape.stroke->width > 0)) proceedStrokes(context, rshape.stroke, path_buff, pool);
    }, rshape.trimpath());

    // update shapes bbox (with empty path handling)
    if ((this->meshGroupShapesBBox.meshes.count > 0 ) ||
        (this->meshGroupStrokesBBox.meshes.count > 0)) {
        updateAABB(tr);
        meshDataBBox.bbox(context, pMin, pMax);
    } else aabb = {{0, 0}, {0, 0}};

    pool->retVertexBuffer(pbuff);
}


void GlRenderDataShape::proceedStrokes(GlContext& context, const RenderStroke* rstroke, const GlVertexBuffer& buff, GlGeometryBufferPool* pool)
{
    assert(rstroke);
    auto strokesGenerator = pool->reqIndexedVertexBuffer(buff.scale);
    if (rstroke->dashPattern) strokesGenerator->appendStrokesDashed(buff, rstroke);
    else strokesGenerator->appendStrokes(buff, rstroke);

    appendStroke(context, *strokesGenerator);

    pool->retIndexedVertexBuffer(strokesGenerator);
}


void GlRenderDataShape::releaseMeshes(GlContext& context)
{
    meshGroupStrokesBBox.release(context);
    meshGroupStrokes.release(context);
    meshGroupShapesBBox.release(context);
    meshGroupShapes.release(context);
    pMin = {FLT_MAX, FLT_MAX};
    pMax = {0.0f, 0.0f};
    aabb = {{0, 0}, {0, 0}};
    clips.clear();
}


void GlRenderDataShape::release(GlContext& context)
{
    releaseMeshes(context);
    meshDataBBox.release(context);
    renderSettingsStroke.release(context);
    renderSettingsShape.release(context);
    GlRenderDataPaint::release(context);
};

//***********************************************************************
// GlRenderDataShapePool
//***********************************************************************

GlRenderDataShape* GlRenderDataShapePool::allocate(GlContext& context)
{
    GlRenderDataShape* renderData{};
    if (mPool.count > 0) {
        renderData = mPool.last();
        mPool.pop();
    } else {
        renderData = new GlRenderDataShape();
        mList.push(renderData);
    }
    return renderData;
}


void GlRenderDataShapePool::free(GlContext& context, GlRenderDataShape* renderData)
{
    renderData->meshGroupShapes.release(context);
    renderData->meshGroupShapesBBox.release(context);
    renderData->meshGroupStrokes.release(context);
    renderData->meshGroupStrokesBBox.release(context);
    renderData->clips.clear();
    mPool.push(renderData);
}


void GlRenderDataShapePool::release(GlContext& context)
{
    ARRAY_FOREACH(p, mList) {
        (*p)->release(context);
        delete(*p);
    }
    mPool.clear();
    mList.clear();
}

//***********************************************************************
// GlRenderDataPicture
//***********************************************************************

void GlRenderDataPicture::updateSurface(GlContext& context, const RenderSurface* surface)
{
    meshData.imageBox(context, surface->w, surface->h);
    imageData.update(context, surface);
}


void GlRenderDataPicture::release(GlContext& context)
{
    imageData.release(context);
    meshData.release(context);
    GlRenderDataPaint::release(context);
}

//***********************************************************************
// GlRenderDataPicturePool
//***********************************************************************

GlRenderDataPicture* GlRenderDataPicturePool::allocate(GlContext& context)
{
    GlRenderDataPicture* renderData{};
    if (mPool.count > 0) {
        renderData = mPool.last();
        mPool.pop();
    } else {
        renderData = new GlRenderDataPicture();
        mList.push(renderData);
    }
    return renderData;
}


void GlRenderDataPicturePool::free(GlContext& context, GlRenderDataPicture* renderData)
{
    renderData->clips.clear();
    mPool.push(renderData);
}


void GlRenderDataPicturePool::release(GlContext& context)
{
    ARRAY_FOREACH(p, mList) {
        (*p)->release(context);
        delete(*p);
    }
    mPool.clear();
    mList.clear();
}

//***********************************************************************
// GlRenderDataStagedBuffer
//***********************************************************************

void GlRenderDataStagedBuffer::append(GlMeshData* meshData)
{
    assert(meshData);
    uint32_t vsize = meshData->vbuffer.count * sizeof(meshData->vbuffer[0]);
    uint32_t tsize = meshData->tbuffer.count * sizeof(meshData->tbuffer[0]);
    uint32_t isize = meshData->ibuffer.count * sizeof(meshData->ibuffer[0]);
    // append vertex data
    if (vbuffer.reserved < vbuffer.count + vsize)
        vbuffer.grow(std::max(vsize, vbuffer.reserved));
    if (meshData->vbuffer.count > 0) {
        meshData->voffset = vbuffer.count;
        memcpy(vbuffer.data + vbuffer.count, meshData->vbuffer.data, vsize);
        vbuffer.count += vsize;
    }
    // append tex coords data
    if (vbuffer.reserved < vbuffer.count + tsize)
        vbuffer.grow(std::max(tsize, vbuffer.reserved));
    if (meshData->tbuffer.count > 0) {
        meshData->toffset = vbuffer.count;
        memcpy(vbuffer.data + vbuffer.count, meshData->tbuffer.data, tsize);
        vbuffer.count += tsize;
    }
    // append index data
    if (ibuffer.reserved < ibuffer.count + isize)
        ibuffer.grow(std::max(isize, ibuffer.reserved));
    if (meshData->ibuffer.count > 0) {
        meshData->ioffset = ibuffer.count;
        memcpy(ibuffer.data + ibuffer.count, meshData->ibuffer.data, isize);
        ibuffer.count += isize;
    }
}


void GlRenderDataStagedBuffer::append(GlMeshDataGroup* meshDataGroup)
{
    ARRAY_FOREACH(p, meshDataGroup->meshes) append(*p);
}


void GlRenderDataStagedBuffer::append(GlRenderDataShape* renderDataShape)
{
    append(&renderDataShape->meshGroupShapes);
    append(&renderDataShape->meshGroupShapesBBox);
    append(&renderDataShape->meshGroupStrokes);
    append(&renderDataShape->meshGroupStrokesBBox);
    append(&renderDataShape->meshDataBBox);
}


void GlRenderDataStagedBuffer::append(GlRenderDataPicture* renderDataPicture)
{
    append(&renderDataPicture->meshData);
}


void GlRenderDataStagedBuffer::initialize(GlContext& context)
{
    // create vertex and index buffers
    vbuffer_gpu = context.createBuffer();
    ibuffer_gpu = context.createBuffer();
    // create vertex array objects
    GL_CHECK(glGenVertexArrays(1, &vao));
}


void GlRenderDataStagedBuffer::release(GlContext& context)
{
    context.releaseBuffer(vbuffer_gpu);
    context.releaseBuffer(ibuffer_gpu);
    GL_CHECK(glDeleteVertexArrays(1, &vao));
}


void GlRenderDataStagedBuffer::clear()
{
    vbuffer.clear();
    ibuffer.clear();
}


void GlRenderDataStagedBuffer::flush(GlContext& context)
{
    // flush data to gpu
    context.allocateBufferVertex(vbuffer_gpu, (float *)vbuffer.data, vbuffer.count);
    context.allocateBufferIndex(ibuffer_gpu, (uint32_t *)ibuffer.data, ibuffer.count);
}


void GlRenderDataStagedBuffer::bind(size_t voffset, size_t toffset)
{
    // re-apply vertex array objects buffers data
    GL_CHECK(glBindVertexArray(vao));
    // set vertex attrib buffer
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbuffer_gpu));
    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (const void*)voffset));
    // set tex coords attrib buffer
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbuffer_gpu));
    GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (const void*)toffset));
    // set index buffer
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer_gpu));
}
