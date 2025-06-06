/*
 * Copyright (c) 2020 - 2025 the ThorVG project. All rights reserved.

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

#include "tvgCommon.h"
#include "tvgTaskScheduler.h"
#include "tvgLoader.h"

#ifdef THORVG_SW_RASTER_SUPPORT
    #include "tvgSwRenderer.h"
#endif

#ifdef THORVG_GL_RASTER_SUPPORT
    #include "tvgGlRenderer.h"
#endif

#ifdef THORVG_WG_RASTER_SUPPORT
    #include "tvgWgRenderer.h"
#endif


/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

namespace tvg {
    int engineInit = 0;
}

static uint16_t _version = 0;


static bool _buildVersionInfo(uint32_t* major, uint32_t* minor, uint32_t* micro)
{
    auto VER = THORVG_VERSION_STRING;
    auto p = VER;
    const char* x;

    if (!(x = strchr(p, '.'))) return false;
    uint32_t majorVal = atoi(p);
    p = x + 1;

    if (!(x = strchr(p, '.'))) return false;
    uint32_t minorVal = atoi(p);
    p = x + 1;

    uint32_t microVal = atoi(p);

    char sum[7];
    snprintf(sum, sizeof(sum), "%d%02d%02d", majorVal, minorVal, microVal);
    _version = atoi(sum);

    if (major) *major = majorVal;
    if (minor) *minor = minorVal;
    if (micro) *micro = microVal;

    return true;
}


/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

Result Initializer::init(uint32_t threads) noexcept
{
    if (engineInit++ > 0) return Result::Success;

    if (!_buildVersionInfo(nullptr, nullptr, nullptr)) return Result::Unknown;

    if (!LoaderMgr::init()) return Result::Unknown;

    TaskScheduler::init(threads);

    return Result::Success;
}


Result Initializer::term() noexcept
{
    if (engineInit == 0) return Result::InsufficientCondition;

    if (--engineInit > 0) return Result::Success;

    #ifdef THORVG_SW_RASTER_SUPPORT
        if (!SwRenderer::term()) return Result::InsufficientCondition;
    #endif

    #ifdef THORVG_GL_RASTER_SUPPORT
        if (!GlRenderer::term()) return Result::InsufficientCondition;
    #endif

    #ifdef THORVG_WG_RASTER_SUPPORT
        if (!WgRenderer::term()) return Result::InsufficientCondition;
    #endif

    TaskScheduler::term();

    if (!LoaderMgr::term()) return Result::Unknown;

    return Result::Success;
}


const char* Initializer::version(uint32_t* major, uint32_t* minor, uint32_t* micro) noexcept
{
    if ((!major && ! minor && !micro) || _buildVersionInfo(major, minor, micro)) return THORVG_VERSION_STRING;
    return nullptr;
}


uint16_t THORVG_VERSION_NUMBER()
{
    return _version;
}


void* operator new(std::size_t size) {
    return tvg::malloc(size);
}


void operator delete(void* ptr) noexcept {
    tvg::free(ptr);
}