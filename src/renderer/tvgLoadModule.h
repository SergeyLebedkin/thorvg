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

#ifndef _TVG_LOAD_MODULE_H_
#define _TVG_LOAD_MODULE_H_

#include <atomic>
#include "tvgCommon.h"
#include "tvgRender.h"
#include "tvgInlist.h"


struct LoadModule
{
    INLIST_ITEM(LoadModule);

    //Use either hashkey(data) or hashpath(path)
    uintptr_t hashkey = 0;
    char* hashpath = nullptr;

    FileType type;                                  //current loader file type
    atomic<uint16_t> sharing{};                     //reference count
    bool readied = false;                           //read done already.
    bool cached = false;                            //cached for sharing

    LoadModule(FileType type) : type(type) {}
    virtual ~LoadModule()
    {
        tvg::free(hashpath);
    }

    void cache(uintptr_t data)
    {
        hashkey = data;
        cached = true;
    }

    void cache(char* data)
    {
        hashpath = data;
        cached = true;
    }

    virtual bool open(const char* path) { return false; }
    virtual bool open(const char* data, uint32_t size, const char* rpath, bool copy) { return false; }
    virtual bool resize(Paint* paint, float w, float h) { return false; }
    virtual void sync() {};  //finish immediately if any async update jobs.

    virtual bool read()
    {
        if (readied) return false;
        readied = true;
        return true;
    }

    virtual bool close()
    {
        if (sharing == 0) return true;
        --sharing;
        return false;
    }
};


struct ImageLoader : LoadModule
{
    static atomic<ColorSpace> cs;                   //desired value

    float w = 0, h = 0;                             //default image size
    RenderSurface surface;

    ImageLoader(FileType type) : LoadModule(type) {}

    virtual bool animatable() { return false; }  //true if this loader supports animation.
    virtual Paint* paint() { return nullptr; }

    virtual RenderSurface* bitmap()
    {
        if (surface.data) return &surface;
        return nullptr;
    }
};


struct FontMetrics
{
    //TODO: add necessary metrics
    float minw;
};


struct FontLoader : LoadModule
{
    char* name = nullptr;

    FontLoader(FileType type) : LoadModule(type) {}

    using LoadModule::read;

    virtual bool read(Shape* shape, char* text, FontMetrics& out) = 0;
    virtual float transform(Paint* paint, FontMetrics& mertrics, float fontSize, bool italic) = 0;
};

#endif //_TVG_LOAD_MODULE_H_
