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

#ifndef _TVG_GL_RENDER_DATA_H_
#define _TVG_GL_RENDER_DATA_H_

#include "tvgGlCommon.h"
#include "tvgGlGeometry.h"
#include "tvgGlShaderTypes.h"

struct GlMeshData {
    Array<Point> vbuffer;
    Array<Point> tbuffer;
    Array<uint32_t> ibuffer;
    size_t voffset{};
    size_t toffset{};
    size_t ioffset{};

    void update(GlContext& context, const GlVertexBuffer& vertexBuffer);
    void update(GlContext& context, const GlIndexedVertexBuffer& vertexBufferInd);
    void bbox(GlContext& context, const Point pmin, const Point pmax);
    void imageBox(GlContext& context, float w, float h);
    void blitBox(GlContext& context);
    void release(GlContext& context) {};
};

class GlMeshDataPool {
private:
    Array<GlMeshData*> mPool;
    Array<GlMeshData*> mList;
public:
    static GlMeshDataPool* gMeshDataPool;
    GlMeshData* allocate(GlContext& context);
    void free(GlContext& context, GlMeshData* meshData);
    void release(GlContext& context);
};

struct GlMeshDataGroup {
    Array<GlMeshData*> meshes{};
    
    void append(GlContext& context, const GlVertexBuffer& vertexBuffer);
    void append(GlContext& context, const GlIndexedVertexBuffer& vertexBufferInd);
    void append(GlContext& context, const Point pmin, const Point pmax);
    void release(GlContext& context);
};

struct GlImageData {
    GLuint texture{};

    void update(GlContext& context, const RenderSurface* surface);
    void release(GlContext& context);
};

enum class GlRenderSettingsType { None = 0, Solid = 1, Linear = 2, Radial = 3 };
enum class GlRenderRasterType { Solid = 0, Gradient, Image };

struct GlRenderSettings
{
    GlShaderTypeVec4f solidColor{};
    GlShaderTypeMat4x4f gradientTrans;
    GlShaderTypeGradient gradient{};

    GLuint sampler{}; // external handle
    GLuint texGradient{};
    
    GlRenderSettingsType fillType{};
    GlRenderRasterType rasterType{};
    bool skip{};

    void updateFill(GlContext& context, const Fill* fill);
    void updateColor(GlContext& context, const RenderColor& c);
    void release(GlContext& context);
};

struct GlRenderDataPaint
{
    GlShaderTypeMat4x4f modelMat{};
    GlShaderTypeVec4f blendSettings{};
    RenderRegion viewport{};
    BBox aabb{{},{}};
    float opacity{};
    Array<GlRenderDataPaint*> clips;

    virtual ~GlRenderDataPaint() {};
    virtual void release(GlContext& context);
    virtual Type type() { return Type::Undefined; };

    void update(GlContext& context, const tvg::Matrix& transform, tvg::ColorSpace cs, uint8_t opacity);
    void updateClips(tvg::Array<tvg::RenderData> &clips);
};

struct GlRenderDataShape: public GlRenderDataPaint
{
    GlRenderSettings renderSettingsShape{};
    GlRenderSettings renderSettingsStroke{};
    GlMeshDataGroup meshGroupShapes{};
    GlMeshDataGroup meshGroupShapesBBox{};
    GlMeshDataGroup meshGroupStrokes{};
    GlMeshDataGroup meshGroupStrokesBBox{};
    GlMeshData meshDataBBox{};
    Point pMin{};
    Point pMax{};
    bool strokeFirst{};
    FillRule fillRule{};

    void appendShape(GlContext& context, const GlVertexBuffer& vertexBuffer);
    void appendStroke(GlContext& context, const GlIndexedVertexBuffer& vertexBufferInd);
    void updateBBox(Point pmin, Point pmax);
    void updateAABB(const Matrix& tr);
    void updateMeshes(GlContext& context, const RenderShape& rshape, const Matrix& tr, GlGeometryBufferPool* pool);
    void proceedStrokes(GlContext& context, const RenderStroke* rstroke, const GlVertexBuffer& buff, GlGeometryBufferPool* pool);
    void releaseMeshes(GlContext& context);
    void release(GlContext& context) override;
    Type type() override { return Type::Shape; };
};

class GlRenderDataShapePool {
private:
    Array<GlRenderDataShape*> mPool;
    Array<GlRenderDataShape*> mList;
public:
    GlRenderDataShape* allocate(GlContext& context);
    void free(GlContext& context, GlRenderDataShape* renderData);
    void release(GlContext& context);
};

struct GlRenderDataPicture: public GlRenderDataPaint
{
    GlImageData imageData{};
    GlMeshData meshData{};

    void updateSurface(GlContext& context, const RenderSurface* surface);
    void release(GlContext& context) override;
    Type type() override { return Type::Picture; };
};

class GlRenderDataPicturePool {
private:
    Array<GlRenderDataPicture*> mPool;
    Array<GlRenderDataPicture*> mList;
public:
    GlRenderDataPicture* allocate(GlContext& context);
    void free(GlContext& context, GlRenderDataPicture* dataPicture);
    void release(GlContext& context);
};

class GlRenderDataStagedBuffer {
private:
    GLuint vbuffer_gpu{};
    GLuint tbuffer_gpu{};
    GLuint ibuffer_gpu{};
    Array<uint8_t> vbuffer;
    Array<uint8_t> tbuffer;
    Array<uint8_t> ibuffer;
public:
    void append(GlMeshData* meshData);
    void append(GlMeshDataGroup* meshDataGroup);
    void append(GlRenderDataShape* renderDataShape);
    void append(GlRenderDataPicture* renderDataPicture);
    void release(GlContext& context);
    void clear();
    void flush(GlContext& context);
    void bind();
};

#endif // _TVG_GL_RENDER_DATA_H_
