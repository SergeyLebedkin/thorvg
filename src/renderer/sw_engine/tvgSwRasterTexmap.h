/*
 * Copyright (c) 2021 - 2025 the ThorVG project. All rights reserved.

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

struct Vertex
{
   Point pt;
   Point uv;
};

struct Polygon
{
   Vertex vertex[3];
};

struct AALine
{
   int32_t x[2];
   int32_t coverage[2];
   int32_t length[2];
};

struct AASpans
{
   AALine *lines;
   int32_t yStart;
   int32_t yEnd;
};

//Careful! Shared resource, No support threading
static float dudx, dvdx;
static float dxdya, dxdyb, dudya, dvdya;
static float xa, xb, ua, va;


static inline int32_t _modf(float v)
{
    return 255 - ((int(v * 256.0f)) & 255);
}


static bool _rasterMaskedPolygonImageSegment(SwSurface* surface, const SwImage& image, const RenderRegion& bbox, int yStart, int yEnd, AASpans* aaSpans, uint8_t opacity, uint8_t dirFlag = 0)
{
    TVGERR("SW_ENGINE", "TODO: _rasterMaskedPolygonImageSegment()");
    return false;
}


static void _rasterBlendingPolygonImageSegment(SwSurface* surface, const SwImage& image, const RenderRegion& bbox, int yStart, int yEnd, AASpans* aaSpans, uint8_t opacity)
{
    float _dudx = dudx, _dvdx = dvdx;
    float _dxdya = dxdya, _dxdyb = dxdyb, _dudya = dudya, _dvdya = dvdya;
    float _xa = xa, _xb = xb, _ua = ua, _va = va;
    auto sbuf = image.buf32;
    auto dbuf = surface->buf32;
    int32_t sw = static_cast<int32_t>(image.w);
    int32_t sh = static_cast<int32_t>(image.h);
    int32_t x1, x2, x, y, ar, ab, iru, irv, px, ay;
    int32_t vv = 0, uu = 0;
    float dx, u, v;
    uint32_t* buf;

    if (yStart < bbox.min.y) yStart = bbox.min.y;
    if (yEnd > bbox.max.y) yEnd = bbox.max.y;

    y = yStart;

    while (y < yEnd) {
        x1 = std::max((int32_t)_xa, bbox.min.x);
        x2 = std::min((int32_t)_xb, bbox.max.x);

        //Anti-Aliasing frames
        if (aaSpans) {
            ay = y - aaSpans->yStart;
            if (aaSpans->lines[ay].x[0] > x1) aaSpans->lines[ay].x[0] = x1;
            if (aaSpans->lines[ay].x[1] < x2) aaSpans->lines[ay].x[1] = x2;
        }

        //Range allowed
        if ((x2 - x1) >= 1 && (x1 < bbox.max.x) && (x2 > bbox.min.x)) {

            //Perform subtexel pre-stepping on UV
            dx = 1 - (_xa - x1);
            u = _ua + dx * _dudx;
            v = _va + dx * _dvdx;
            buf = dbuf + ((y * surface->stride) + x1);
            x = x1;

            //Draw horizontal line
            while (x++ < x2) {
                uu = (int) u;
                vv = (int) v;

                if ((uint32_t) uu >= image.w || (uint32_t) vv >= image.h) continue;

                ar = _modf(u);
                ab = _modf(v);
                iru = uu + 1;
                irv = vv + 1;

                px = *(sbuf + (vv * image.stride) + uu);

                /* horizontal interpolate */
                if (iru < sw) {
                    /* right pixel */
                    int px2 = *(sbuf + (vv * image.stride) + iru);
                    px = INTERPOLATE(px, px2, ar);
                }
                /* vertical interpolate */
                if (irv < sh) {
                    /* bottom pixel */
                    int px2 = *(sbuf + (irv * image.stride) + uu);
                    /* horizontal interpolate */
                    if (iru < sw) {
                        /* bottom right pixel */
                        int px3 = *(sbuf + (irv * image.stride) + iru);
                        px2 = INTERPOLATE(px2, px3, ar);
                    }
                    px = INTERPOLATE(px, px2, ab);
                }
                *buf = INTERPOLATE(surface->blender(rasterUnpremultiply(px), *buf), *buf, MULTIPLY(opacity, A(px)));
                ++buf;

                //Step UV horizontally
                u += _dudx;
                v += _dvdx;
            }
        }

        //Step along both edges
        _xa += _dxdya;
        _xb += _dxdyb;
        _ua += _dudya;
        _va += _dvdya;

        ++y;
    }
    xa = _xa;
    xb = _xb;
    ua = _ua;
    va = _va;
}


static void _rasterPolygonImageSegment(SwSurface* surface, const SwImage& image, const RenderRegion& bbox, int yStart, int yEnd, AASpans* aaSpans, uint8_t opacity, bool matting)
{
    float _dudx = dudx, _dvdx = dvdx;
    float _dxdya = dxdya, _dxdyb = dxdyb, _dudya = dudya, _dvdya = dvdya;
    float _xa = xa, _xb = xb, _ua = ua, _va = va;
    auto sbuf = image.buf32;
    auto dbuf = surface->buf32;
    int32_t sw = static_cast<int32_t>(image.w);
    int32_t sh = static_cast<int32_t>(image.h);
    int32_t x1, x2, x, y, ar, ab, iru, irv, px, ay;
    int32_t vv = 0, uu = 0;
    float dx, u, v;
    uint32_t* buf;

    //for matting(composition)
    auto csize = matting ? surface->compositor->image.channelSize: 0;
    auto alpha = matting ? surface->alpha(surface->compositor->method) : nullptr;
    uint8_t* cmp = nullptr;

    if (yStart < bbox.min.y) yStart = bbox.min.y;
    if (yEnd > bbox.max.y) yEnd = bbox.max.y;

    y = yStart;

    while (y < yEnd) {
        x1 = std::max((int32_t)_xa, bbox.min.x);
        x2 = std::min((int32_t)_xb, bbox.max.x);

        //Anti-Aliasing frames
        if (aaSpans) {
            ay = y - aaSpans->yStart;
            if (aaSpans->lines[ay].x[0] > x1) aaSpans->lines[ay].x[0] = x1;
            if (aaSpans->lines[ay].x[1] < x2) aaSpans->lines[ay].x[1] = x2;
        }

        //Range allowed
        if ((x2 - x1) >= 1 && (x1 < bbox.max.x) && (x2 > bbox.min.x)) {

            //Perform subtexel pre-stepping on UV
            dx = 1 - (_xa - x1);
            u = _ua + dx * _dudx;
            v = _va + dx * _dvdx;
            buf = dbuf + ((y * surface->stride) + x1);
            x = x1;

            if (matting) cmp = &surface->compositor->image.buf8[(y * surface->compositor->image.stride + x1) * csize];

            const auto fullOpacity = (opacity == 255);

            //Draw horizontal line
            while (x++ < x2) {
                uu = (int) u;
                vv = (int) v;

                if ((uint32_t) uu >= image.w || (uint32_t) vv >= image.h) continue;

                ar = _modf(u);
                ab = _modf(v);
                iru = uu + 1;
                irv = vv + 1;

                px = *(sbuf + (vv * image.stride) + uu);

                /* horizontal interpolate */
                if (iru < sw) {
                    /* right pixel */
                    int px2 = *(sbuf + (vv * image.stride) + iru);
                    px = INTERPOLATE(px, px2, ar);
                }
                /* vertical interpolate */
                if (irv < sh) {
                    /* bottom pixel */
                    int px2 = *(sbuf + (irv * image.stride) + uu);

                    /* horizontal interpolate */
                    if (iru < sw) {
                        /* bottom right pixel */
                        int px3 = *(sbuf + (irv * image.stride) + iru);
                        px2 = INTERPOLATE(px2, px3, ar);
                    }
                    px = INTERPOLATE(px, px2, ab);
                }
                uint32_t src;
                if (matting) {
                    auto a = alpha(cmp);
                    src = fullOpacity ? ALPHA_BLEND(px, a) : ALPHA_BLEND(px, MULTIPLY(opacity, a));
                    cmp += csize;
                } else {
                    src = fullOpacity ? px : ALPHA_BLEND(px, opacity);
                }
                *buf = src + ALPHA_BLEND(*buf, IA(src));
                ++buf;

                //Step UV horizontally
                u += _dudx;
                v += _dvdx;
            }
        }

        //Step along both edges
        _xa += _dxdya;
        _xb += _dxdyb;
        _ua += _dudya;
        _va += _dvdya;

        ++y;
    }
    xa = _xa;
    xb = _xb;
    ua = _ua;
    va = _va;
}


/* This mapping algorithm is based on Mikael Kalms's. */
static void _rasterPolygonImage(SwSurface* surface, const SwImage& image, const RenderRegion& bbox, Polygon& polygon, AASpans* aaSpans, uint8_t opacity)
{
    float x[3] = {polygon.vertex[0].pt.x, polygon.vertex[1].pt.x, polygon.vertex[2].pt.x};
    float y[3] = {polygon.vertex[0].pt.y, polygon.vertex[1].pt.y, polygon.vertex[2].pt.y};
    float u[3] = {polygon.vertex[0].uv.x, polygon.vertex[1].uv.x, polygon.vertex[2].uv.x};
    float v[3] = {polygon.vertex[0].uv.y, polygon.vertex[1].uv.y, polygon.vertex[2].uv.y};

    float off_y;
    float dxdy[3] = {0.0f, 0.0f, 0.0f};

    auto upper = false;

    //Sort the vertices in ascending Y order
    if (y[0] > y[1]) {
        std::swap(x[0], x[1]);
        std::swap(y[0], y[1]);
        std::swap(u[0], u[1]);
        std::swap(v[0], v[1]);
    }
    if (y[0] > y[2])  {
        std::swap(x[0], x[2]);
        std::swap(y[0], y[2]);
        std::swap(u[0], u[2]);
        std::swap(v[0], v[2]);
    }
    if (y[1] > y[2]) {
        std::swap(x[1], x[2]);
        std::swap(y[1], y[2]);
        std::swap(u[1], u[2]);
        std::swap(v[1], v[2]);
    }

    //Y indexes
    int yi[3] = {(int)y[0], (int)y[1], (int)y[2]};

    //Skip drawing if it's too thin to cover any pixels at all.
    if ((yi[0] == yi[1] && yi[0] == yi[2]) || ((int) x[0] == (int) x[1] && (int) x[0] == (int) x[2])) return;

    //Calculate horizontal and vertical increments for UV axes (these calcs are certainly not optimal, although they're stable (handles any dy being 0)
    auto denom = ((x[2] - x[0]) * (y[1] - y[0]) - (x[1] - x[0]) * (y[2] - y[0]));

    //Skip poly if it's an infinitely thin line
    if (tvg::zero(denom)) return;

    denom = 1 / denom;   //Reciprocal for speeding up
    dudx = ((u[2] - u[0]) * (y[1] - y[0]) - (u[1] - u[0]) * (y[2] - y[0])) * denom;
    dvdx = ((v[2] - v[0]) * (y[1] - y[0]) - (v[1] - v[0]) * (y[2] - y[0])) * denom;
    auto dudy = ((u[1] - u[0]) * (x[2] - x[0]) - (u[2] - u[0]) * (x[1] - x[0])) * denom;
    auto dvdy = ((v[1] - v[0]) * (x[2] - x[0]) - (v[2] - v[0]) * (x[1] - x[0])) * denom;

    //Calculate X-slopes along the edges
    if (y[1] > y[0]) dxdy[0] = (x[1] - x[0]) / (y[1] - y[0]);
    if (y[2] > y[0]) dxdy[1] = (x[2] - x[0]) / (y[2] - y[0]);
    if (y[2] > y[1]) dxdy[2] = (x[2] - x[1]) / (y[2] - y[1]);

    //Determine which side of the polygon the longer edge is on
    auto side = (dxdy[1] > dxdy[0]) ? true : false;

    if (tvg::equal(y[0], y[1])) side = x[0] > x[1];
    if (tvg::equal(y[1], y[2])) side = x[2] > x[1];

    auto compositing = _compositing(surface);   //Composition required
    auto blending = _blending(surface);         //Blending required

    //Longer edge is on the left side
    if (!side) {
        //Calculate slopes along left edge
        dxdya = dxdy[1];
        dudya = dxdya * dudx + dudy;
        dvdya = dxdya * dvdx + dvdy;

        //Perform subpixel pre-stepping along left edge
        auto dy = 1.0f - (y[0] - yi[0]);
        xa = x[0] + dy * dxdya;
        ua = u[0] + dy * dudya;
        va = v[0] + dy * dvdya;

        //Draw upper segment if possibly visible
        if (yi[0] < yi[1]) {
            off_y = y[0] < bbox.min.y ? (bbox.min.y - y[0]) : 0;
            xa += (off_y * dxdya);
            ua += (off_y * dudya);
            va += (off_y * dvdya);

            // Set right edge X-slope and perform subpixel pre-stepping
            dxdyb = dxdy[0];
            xb = x[0] + dy * dxdyb + (off_y * dxdyb);

            if (compositing) {
                if (_matting(surface)) _rasterPolygonImageSegment(surface, image, bbox, yi[0], yi[1], aaSpans, opacity, true);
                else _rasterMaskedPolygonImageSegment(surface, image, bbox, yi[0], yi[1], aaSpans, opacity, 1);
            } else if (blending) {
                _rasterBlendingPolygonImageSegment(surface, image, bbox, yi[0], yi[1], aaSpans, opacity);
            } else {
                _rasterPolygonImageSegment(surface, image, bbox, yi[0], yi[1], aaSpans, opacity, false);
            }
            upper = true;
        }
        //Draw lower segment if possibly visible
        if (yi[1] < yi[2]) {
            off_y = y[1] < bbox.min.y ? (bbox.min.y - y[1]) : 0;
            if (!upper) {
                xa += (off_y * dxdya);
                ua += (off_y * dudya);
                va += (off_y * dvdya);
            }
            // Set right edge X-slope and perform subpixel pre-stepping
            dxdyb = dxdy[2];
            xb = x[1] + (1 - (y[1] - yi[1])) * dxdyb + (off_y * dxdyb);
            if (compositing) {
                if (_matting(surface)) _rasterPolygonImageSegment(surface, image, bbox, yi[1], yi[2], aaSpans, opacity, true);
                else _rasterMaskedPolygonImageSegment(surface, image, bbox, yi[1], yi[2], aaSpans, opacity, 2);
            } else if (blending) {
                 _rasterBlendingPolygonImageSegment(surface, image, bbox, yi[1], yi[2], aaSpans, opacity);
            } else {
                _rasterPolygonImageSegment(surface, image, bbox, yi[1], yi[2], aaSpans, opacity, false);
            }
        }
    //Longer edge is on the right side
    } else {
        //Set right edge X-slope and perform subpixel pre-stepping
        dxdyb = dxdy[1];
        auto dy = 1.0f - (y[0] - yi[0]);
        xb = x[0] + dy * dxdyb;

        //Draw upper segment if possibly visible
        if (yi[0] < yi[1]) {
            off_y = y[0] < bbox.min.y ? (bbox.min.y - y[0]) : 0;
            xb += (off_y *dxdyb);

            // Set slopes along left edge and perform subpixel pre-stepping
            dxdya = dxdy[0];
            dudya = dxdya * dudx + dudy;
            dvdya = dxdya * dvdx + dvdy;

            xa = x[0] + dy * dxdya + (off_y * dxdya);
            ua = u[0] + dy * dudya + (off_y * dudya);
            va = v[0] + dy * dvdya + (off_y * dvdya);

            if (compositing) {
                if (_matting(surface)) _rasterPolygonImageSegment(surface, image, bbox, yi[0], yi[1], aaSpans, opacity, true);
                else _rasterMaskedPolygonImageSegment(surface, image, bbox, yi[0], yi[1], aaSpans, opacity, 3);
            } else if (blending) {
                _rasterBlendingPolygonImageSegment(surface, image, bbox, yi[0], yi[1], aaSpans, opacity);
            } else {
                _rasterPolygonImageSegment(surface, image, bbox, yi[0], yi[1], aaSpans, opacity, false);
            }
            upper = true;
        }
        //Draw lower segment if possibly visible
        if (yi[1] < yi[2]) {
            off_y = y[1] < bbox.min.y ? (bbox.min.y - y[1]) : 0;
            if (!upper) xb += (off_y *dxdyb);

            // Set slopes along left edge and perform subpixel pre-stepping
            dxdya = dxdy[2];
            dudya = dxdya * dudx + dudy;
            dvdya = dxdya * dvdx + dvdy;
            dy = 1 - (y[1] - yi[1]);
            xa = x[1] + dy * dxdya + (off_y * dxdya);
            ua = u[1] + dy * dudya + (off_y * dudya);
            va = v[1] + dy * dvdya + (off_y * dvdya);

            if (compositing) {
                if (_matting(surface)) _rasterPolygonImageSegment(surface, image, bbox, yi[1], yi[2], aaSpans, opacity, true);
                else _rasterMaskedPolygonImageSegment(surface, image, bbox, yi[1], yi[2], aaSpans, opacity, 4);
            } else if (blending) {
                _rasterBlendingPolygonImageSegment(surface, image, bbox, yi[1], yi[2], aaSpans, opacity);
            } else {
                _rasterPolygonImageSegment(surface, image, bbox, yi[1], yi[2], aaSpans, opacity, false);
            }
        }
    }
}


static AASpans* _AASpans(int yStart, int yEnd)
{
    auto aaSpans = tvg::malloc<AASpans*>(sizeof(AASpans));
    aaSpans->yStart = yStart;
    aaSpans->yEnd = yEnd;

    //Initialize X range
    auto height = yEnd - yStart;

    aaSpans->lines = tvg::malloc<AALine*>(height * sizeof(AALine));

    for (int32_t i = 0; i < height; i++) {
        aaSpans->lines[i].x[0] = INT32_MAX;
        aaSpans->lines[i].x[1] = 0;
        aaSpans->lines[i].length[0] = 0;
        aaSpans->lines[i].length[1] = 0;
    }
    return aaSpans;
}


static void _calcIrregularCoverage(AALine* lines, int32_t eidx, int32_t y, int32_t diagonal, int32_t edgeDist, bool reverse)
{
    if (eidx == 1) reverse = !reverse;
    int32_t coverage = (255 / (diagonal + 2));
    int32_t tmp;
    for (int32_t ry = 0; ry < (diagonal + 2); ry++) {
        tmp = y - ry - edgeDist;
        if (tmp < 0) return;
        lines[tmp].length[eidx] = 1;
        if (reverse) lines[tmp].coverage[eidx] = 255 - (coverage * ry);
        else lines[tmp].coverage[eidx] = (coverage * ry);
    }
}


static void _calcVertCoverage(AALine *lines, int32_t eidx, int32_t y, int32_t rewind, bool reverse)
{
    if (eidx == 1) reverse = !reverse;
    int32_t coverage = (255 / (rewind + 1));
    int32_t tmp;
    for (int ry = 1; ry < (rewind + 1); ry++) {
        tmp = y - ry;
        if (tmp < 0) return;
        lines[tmp].length[eidx] = 1;
        if (reverse) lines[tmp].coverage[eidx] = (255 - (coverage * ry));
        else lines[tmp].coverage[eidx] = (coverage * ry);
    }
}


static void _calcHorizCoverage(AALine *lines, int32_t eidx, int32_t y, int32_t x, int32_t x2)
{
    lines[y].length[eidx] = abs(x - x2);
    lines[y].coverage[eidx] = (255 / (lines[y].length[eidx] + 1));
}


/*
 * This Anti-Aliasing mechanism is originated from Hermet Park's idea.
 * To understand this AA logic, you can refer this page:
 * https://uigraphics.tistory.com/1
*/
static void _calcAAEdge(AASpans *aaSpans, int32_t eidx)
{
//Previous edge direction:
#define DirOutHor 0x0011
#define DirOutVer 0x0001
#define DirInHor  0x0010
#define DirInVer  0x0000
#define DirNone   0x1000

#define PUSH_VERTEX() \
    do { \
        pEdge.x = lines[y].x[eidx]; \
        pEdge.y = y; \
        ptx[0] = tx[0]; \
        ptx[1] = tx[1]; \
    } while (0)

    int32_t y = 0;
    SwPoint pEdge = {-1, -1};       //previous edge point
    SwPoint edgeDiff = {0, 0};      //temporary used for point distance

    /* store bigger to tx[0] between prev and current edge's x positions. */
    int32_t tx[2] = {0, 0};
    /* back up prev tx values */
    int32_t ptx[2] = {0, 0};
    int32_t diagonal = 0;           //straight diagonal pixels count

    auto yStart = aaSpans->yStart;
    auto yEnd = aaSpans->yEnd;
    auto lines = aaSpans->lines;

    int32_t prevDir = DirNone;
    int32_t curDir = DirNone;

    yEnd -= yStart;

    //Start Edge
    if (y < yEnd) {
        pEdge.x = lines[y].x[eidx];
        pEdge.y = y;
    }

    //Calculates AA Edges
    for (y++; y < yEnd; y++) {

        if (lines[y].x[0] == INT32_MAX) continue;

        //Ready tx
        if (eidx == 0) {
            tx[0] = pEdge.x;
            tx[1] = lines[y].x[0];
        } else {
            tx[0] = lines[y].x[1];
            tx[1] = pEdge.x;
        }
        edgeDiff.x = (tx[0] - tx[1]);
        edgeDiff.y = (y - pEdge.y);

        //Confirm current edge direction
        if (edgeDiff.x > 0) {
            if (edgeDiff.y == 1) curDir = DirOutHor;
            else curDir = DirOutVer;
        } else if (edgeDiff.x < 0) {
            if (edgeDiff.y == 1) curDir = DirInHor;
            else curDir = DirInVer;
        } else curDir = DirNone;

        //straight diagonal increase
        if ((curDir == prevDir) && (y < yEnd)) {
            if ((abs(edgeDiff.x) == 1) && (edgeDiff.y == 1)) {
                ++diagonal;
                PUSH_VERTEX();
                continue;
            }
        }

        switch (curDir) {
            case DirOutHor: {
                _calcHorizCoverage(lines, eidx, y, tx[0], tx[1]);
                if (diagonal > 0) {
                    _calcIrregularCoverage(lines, eidx, y, diagonal, 0, true);
                    diagonal = 0;
                }
               /* Increment direction is changed: Outside Vertical -> Outside Horizontal */
               if (prevDir == DirOutVer) _calcHorizCoverage(lines, eidx, pEdge.y, ptx[0], ptx[1]);

               //Trick, but fine-tunning!
               if (y == 1) _calcHorizCoverage(lines, eidx, pEdge.y, tx[0], tx[1]);
               PUSH_VERTEX();
            }
            break;
            case DirOutVer: {
                _calcVertCoverage(lines, eidx, y, edgeDiff.y, true);
                if (diagonal > 0) {
                    _calcIrregularCoverage(lines, eidx, y, diagonal, edgeDiff.y, false);
                    diagonal = 0;
                }
               /* Increment direction is changed: Outside Horizontal -> Outside Vertical */
               if (prevDir == DirOutHor) _calcHorizCoverage(lines, eidx, pEdge.y, ptx[0], ptx[1]);
               PUSH_VERTEX();
            }
            break;
            case DirInHor: {
                _calcHorizCoverage(lines, eidx, (y - 1), tx[0], tx[1]);
                if (diagonal > 0) {
                    _calcIrregularCoverage(lines, eidx, y, diagonal, 0, false);
                    diagonal = 0;
                }
                /* Increment direction is changed: Outside Horizontal -> Inside Horizontal */
               if (prevDir == DirOutHor) _calcHorizCoverage(lines, eidx, pEdge.y, ptx[0], ptx[1]);
               PUSH_VERTEX();
            }
            break;
            case DirInVer: {
                _calcVertCoverage(lines, eidx, y, edgeDiff.y, false);
                if (prevDir == DirOutHor) edgeDiff.y -= 1;      //Weird, fine tuning?????????????????????
                if (diagonal > 0) {
                    _calcIrregularCoverage(lines, eidx, y, diagonal, edgeDiff.y, true);
                    diagonal = 0;
                }
                /* Increment direction is changed: Outside Horizontal -> Inside Vertical */
                if (prevDir == DirOutHor) _calcHorizCoverage(lines, eidx, pEdge.y, ptx[0], ptx[1]);
                PUSH_VERTEX();
            }
            break;
        }
        if (curDir != DirNone) prevDir = curDir;
    }

    //leftovers...?
    if ((edgeDiff.y == 1) && (edgeDiff.x != 0)) {
        if (y >= yEnd) y = (yEnd - 1);
        _calcHorizCoverage(lines, eidx, y - 1, ptx[0], ptx[1]);
        _calcHorizCoverage(lines, eidx, y, tx[0], tx[1]);
    } else {
        ++y;
        if (y > yEnd) y = yEnd;
        _calcVertCoverage(lines, eidx, y, (edgeDiff.y + 1), (prevDir & 0x00000001));
    }
}


static void _apply(SwSurface* surface, AASpans* aaSpans)
{
    auto end = surface->buf32 + surface->h * surface->stride;
    auto buf = surface->buf32 + surface->stride * aaSpans->yStart;
    auto y = aaSpans->yStart;
    auto line = aaSpans->lines;
    uint32_t pix;
    uint32_t* dst;
    int32_t pos;

   _calcAAEdge(aaSpans, 0);    //left side
   _calcAAEdge(aaSpans, 1);    //right side

    while (y < aaSpans->yEnd) {
        if (line->x[1] - line->x[0] > 0) {
            //Left edge
            dst = buf + line->x[0];
            pix = *(dst - ((line->x[0] > 1) ? 1 : 0));
            pos = 1;

            //exceptional handling. out of memory bound.
            if (dst + line->length[0] >= end) {
                pos += (dst + line->length[0] - end);
            }

            while (pos <= line->length[0]) {
                *dst = INTERPOLATE(*dst, pix, line->coverage[0] * pos);
                ++dst;
                ++pos;
            }

            //Right edge
            dst = buf + line->x[1] - 1;
            pix = *(dst + (line->x[1] < (int32_t)(surface->w - 1) ? 1 : 0));
            pos = line->length[1];

            //exceptional handling. out of memory bound.
            if (dst - pos < surface->buf32) --pos;

            while (pos > 0) {
                *dst = INTERPOLATE(*dst, pix, 255 - (line->coverage[1] * pos));
                --dst;
                --pos;
            }
        }
        buf += surface->stride;
        ++line;
        ++y;
    }
    tvg::free(aaSpans->lines);
    tvg::free(aaSpans);
}


/*
    2 triangles constructs 1 mesh.
    below figure illustrates vert[4] index info.
    If you need better quality, please divide a mesh by more number of triangles.

    0 -- 1
    |  / |
    | /  |
    3 -- 2
*/
bool rasterTexmapPolygon(SwSurface* surface, const SwImage& image, const Matrix& transform, const RenderRegion& bbox, uint8_t opacity)
{
    if (surface->channelSize == sizeof(uint8_t)) {
        TVGERR("SW_ENGINE", "Not supported grayscale Textmap polygon!");
        return false;
    }

    //Prepare vertices. Shift XY coordinates to match the sub-pixeling technique.
    Vertex vertices[4];
    vertices[0] = {{0.0f, 0.0f}, {0.0f, 0.0f}};
    vertices[1] = {{float(image.w), 0.0f}, {float(image.w), 0.0f}};
    vertices[2] = {{float(image.w), float(image.h)}, {float(image.w), float(image.h)}};
    vertices[3] = {{0.0f, float(image.h)}, {0.0f, float(image.h)}};

    float ys = FLT_MAX, ye = -1.0f;
    for (int i = 0; i < 4; i++) {
        vertices[i].pt *= transform;
        if (vertices[i].pt.y < ys) ys = vertices[i].pt.y;
        if (vertices[i].pt.y > ye) ye = vertices[i].pt.y;
    }

    auto yStart = std::max(static_cast<int>(ys), bbox.min.y);
    auto yEnd = std::min(static_cast<int>(ye), bbox.max.y);
    auto aaSpans = rightAngle(transform) ?  nullptr : _AASpans(yStart, yEnd);

    Polygon polygon;

    //Draw the first polygon
    polygon.vertex[0] = vertices[0];
    polygon.vertex[1] = vertices[1];
    polygon.vertex[2] = vertices[3];

    _rasterPolygonImage(surface, image, bbox, polygon, aaSpans, opacity);

    //Draw the second polygon
    polygon.vertex[0] = vertices[1];
    polygon.vertex[1] = vertices[2];
    polygon.vertex[2] = vertices[3];

    _rasterPolygonImage(surface, image, bbox, polygon, aaSpans, opacity);

#if 0
    if (_compositing(surface) && _masking(surface) && !_direct(surface->compositor->method)) {
        _compositeMaskImage(surface, &surface->compositor->image, surface->compositor->bbox);
    }
#endif
    if (aaSpans) _apply(surface, aaSpans);
    return true;
}