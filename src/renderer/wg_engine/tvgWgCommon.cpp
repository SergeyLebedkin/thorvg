/*
 * Copyright (c) 2023 - 2024 the ThorVG project. All rights reserved.

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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cassert>
#include "tvgWgCommon.h"
#include "tvgArray.h"

// ****************************************************************************
// WgStagedBuffer
// ****************************************************************************

void WgStagedBuffer::initialize(WGPUDevice device, WGPUQueue queue)
{
    // store context handles
    this->device = device;
    this->queue = queue;
    // pre allocate staged buffer
    allocateBufferCpu(WG_STAGED_BUFFER_MIN_SIZE);
    allocateBufferGpu(WG_STAGED_BUFFER_MIN_SIZE);
};


void WgStagedBuffer::release()
{
    // clear allocated records
    stagedRecords.clear();
    bufferSize = 0;
    allocated = 0;
    // free sys memory buffer
    free(bufferCpu);
    bufferCpu = nullptr;
    // free gpu memory buffer
    wgpuBufferDestroy(bufferGpu);
    wgpuBufferRelease(bufferGpu);
    bufferGpu = nullptr;
    // clear context handles
    queue = nullptr;
    device = nullptr;
}


void WgStagedBuffer::allocateBufferCpu(uint64_t size)
{
    if (bufferSize >= size) return;
    bufferCpu = (uint8_t *)realloc(bufferCpu, size);
    assert(bufferCpu);
    bufferSize = size;
}


void WgStagedBuffer::allocateBufferGpu(uint64_t size)
{
    if ((bufferGpu) && (size <= wgpuBufferGetSize(bufferGpu))) return;
    // release gpu memory buffer
    if (bufferGpu) {
        wgpuBufferDestroy(bufferGpu);
        wgpuBufferRelease(bufferGpu);
        bufferGpu = nullptr;
    }
    // allocate gpu memory buffer
    const WGPUBufferDescriptor bufferDesc { .usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, .size = size };
    bufferGpu = wgpuDeviceCreateBuffer(device, &bufferDesc);
    assert(bufferGpu);
}


void WgStagedBuffer::writeAsync(WGPUBuffer buffer, const void* data, uint64_t size)
{
    // reallocate cpu buffer to requested size if necessary
    allocateBufferCpu(allocated + size);
    // flush data to cpu memory buffer
    stagedRecords.push({ buffer, allocated, size });
    memcpy(bufferCpu + allocated, data, size);
    allocated += size;
}


void WgStagedBuffer::flush(WGPUCommandEncoder commandEncoder)
{
    // reallocate cpu buffer to requested size if necessary
    allocateBufferGpu(bufferSize);
    // flush data to gpu memory buffer
    wgpuQueueWriteBuffer(queue, bufferGpu, 0, bufferCpu, allocated);
    // copy data to requested gpu memory buffers
    for (uint32_t i = 0; i < stagedRecords.count; i++) {
        WGPUBuffer bufferDst = stagedRecords[i].buffer;
        uint64_t offset = stagedRecords[i].offset;
        uint64_t size = stagedRecords[i].size;
        wgpuCommandEncoderCopyBufferToBuffer(commandEncoder, bufferGpu, offset, bufferDst, 0, size);
    }
    // reset staged records
    stagedRecords.clear();
    allocated = 0;
}


// ****************************************************************************
// WgContext
// ****************************************************************************

void WgContext::initialize(WGPUInstance instance, WGPUDevice device)
{
    assert(instance);
    assert(device);
    
    // store global instance and surface
    this->instance = instance;
    this->device = device;
    this->preferredFormat = WGPUTextureFormat_BGRA8Unorm;

    // get current queue
    queue = wgpuDeviceGetQueue(device);
    assert(queue);

    // create shared webgpu assets
    allocateBufferIndexFan(32768);
    samplerNearestRepeat = createSampler(WGPUFilterMode_Nearest, WGPUMipmapFilterMode_Nearest, WGPUAddressMode_Repeat);
    samplerLinearRepeat = createSampler(WGPUFilterMode_Linear, WGPUMipmapFilterMode_Linear, WGPUAddressMode_Repeat, 4);
    samplerLinearMirror = createSampler(WGPUFilterMode_Linear, WGPUMipmapFilterMode_Linear, WGPUAddressMode_MirrorRepeat, 4);
    samplerLinearClamp = createSampler(WGPUFilterMode_Linear, WGPUMipmapFilterMode_Linear, WGPUAddressMode_ClampToEdge, 4);
    assert(samplerNearestRepeat);
    assert(samplerLinearRepeat);
    assert(samplerLinearMirror);
    assert(samplerLinearClamp);

    // initialize bind group layouts
    layouts.initialize(device);
    // initialize staged buffers
    stagedBuffer.initialize(device, queue);
}


void WgContext::release()
{
    stagedBuffer.release();
    layouts.release();
    releaseSampler(samplerLinearClamp);
    releaseSampler(samplerLinearMirror);
    releaseSampler(samplerLinearRepeat);
    releaseSampler(samplerNearestRepeat);
    releaseBuffer(bufferIndexFan);
    releaseQueue(queue);
}


WGPUSampler WgContext::createSampler(WGPUFilterMode filter, WGPUMipmapFilterMode mipmapFilter, WGPUAddressMode addrMode, uint16_t anisotropy)
{
    const WGPUSamplerDescriptor samplerDesc {
        .addressModeU = addrMode, .addressModeV = addrMode, .addressModeW = addrMode,
        .magFilter = filter, .minFilter = filter, .mipmapFilter = mipmapFilter,
        .lodMinClamp = 0.0f, .lodMaxClamp = 32.0f, .maxAnisotropy = anisotropy
    };
    return wgpuDeviceCreateSampler(device, &samplerDesc);
}


bool WgContext::allocateTexture(WGPUTexture& texture, uint32_t width, uint32_t height, WGPUTextureFormat format, void* data)
{
    if ((texture) && (wgpuTextureGetWidth(texture) == width) && (wgpuTextureGetHeight(texture) == height)) {
        // update texture data
        const WGPUImageCopyTexture imageCopyTexture{ .texture = texture };
        const WGPUTextureDataLayout textureDataLayout{ .bytesPerRow = 4 * width, .rowsPerImage = height };
        const WGPUExtent3D writeSize{ .width = width, .height = height, .depthOrArrayLayers = 1 };
        wgpuQueueWriteTexture(queue, &imageCopyTexture, data, 4 * width * height, &textureDataLayout, &writeSize);
        wgpuQueueSubmit(queue, 0, nullptr);
    } else {
        releaseTexture(texture);
        texture = createTexture(width, height, format);
        // update texture data
        const WGPUImageCopyTexture imageCopyTexture{ .texture = texture };
        const WGPUTextureDataLayout textureDataLayout{ .bytesPerRow = 4 * width, .rowsPerImage = height };
        const WGPUExtent3D writeSize{ .width = width, .height = height, .depthOrArrayLayers = 1 };
        wgpuQueueWriteTexture(queue, &imageCopyTexture, data, 4 * width * height, &textureDataLayout, &writeSize);
        wgpuQueueSubmit(queue, 0, nullptr);
        return true;
    }
    return false;

}


WGPUTexture WgContext::createTexture(uint32_t width, uint32_t height, WGPUTextureFormat format)
{
    const WGPUTextureDescriptor textureDesc {
        .usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding,
        .dimension = WGPUTextureDimension_2D, .size = { width, height, 1 },
        .format = format, .mipLevelCount = 1, .sampleCount = 1
    };
    return wgpuDeviceCreateTexture(device, &textureDesc);
}


WGPUTexture WgContext::createTexStorage(uint32_t width, uint32_t height, WGPUTextureFormat format)
{
    const WGPUTextureDescriptor textureDesc {
        .usage = WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding | WGPUTextureUsage_RenderAttachment,
        .dimension = WGPUTextureDimension_2D, .size = { width, height, 1 },
        .format = format, .mipLevelCount = 1, .sampleCount = 1
    };
    return wgpuDeviceCreateTexture(device, &textureDesc);
}


WGPUTexture WgContext::createTexAttachement(uint32_t width, uint32_t height, WGPUTextureFormat format, uint32_t sc)
{
    const WGPUTextureDescriptor textureDesc {
        .usage = WGPUTextureUsage_RenderAttachment,
        .dimension = WGPUTextureDimension_2D, .size = { width, height, 1 },
        .format = format, .mipLevelCount = 1, .sampleCount = sc
    };
    return wgpuDeviceCreateTexture(device, &textureDesc);
}


WGPUTextureView WgContext::createTextureView(WGPUTexture texture)
{
    const WGPUTextureViewDescriptor textureViewDesc {
        .format = wgpuTextureGetFormat(texture),
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_All
    };
    return wgpuTextureCreateView(texture, &textureViewDesc);
}


void WgContext::releaseTextureView(WGPUTextureView& textureView)
{
    if (textureView) {
        wgpuTextureViewRelease(textureView);
        textureView = nullptr;
    }
}


void WgContext::releaseTexture(WGPUTexture& texture)
{
    if (texture) {
        wgpuTextureDestroy(texture);
        wgpuTextureRelease(texture);
        texture = nullptr;
    }
    
}


void WgContext::releaseSampler(WGPUSampler& sampler)
{
    if (sampler) {
        wgpuSamplerRelease(sampler);
        sampler = nullptr;
    }
}


bool WgContext::allocateBufferUniform(WGPUBuffer& buffer, const void* data, uint64_t size)
{
    if ((buffer) && (wgpuBufferGetSize(buffer) >= size)) {
        //wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
        stagedBuffer.writeAsync(buffer, data, size);
    } else {
        releaseBuffer(buffer);
        const WGPUBufferDescriptor bufferDesc { .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, .size = size };
        buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        //wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
        stagedBuffer.writeAsync(buffer, data, size);
        return true;
    }
    return false;
}


bool WgContext::allocateBufferVertex(WGPUBuffer& buffer, const float* data, uint64_t size)
{
    if ((buffer) && (wgpuBufferGetSize(buffer) >= size)) {
        //wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
        stagedBuffer.writeAsync(buffer, data, size);
    } else {
        releaseBuffer(buffer);
        const WGPUBufferDescriptor bufferDesc {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = size > WG_VERTEX_BUFFER_MIN_SIZE ? size : WG_VERTEX_BUFFER_MIN_SIZE
        };
        buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        //wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
        stagedBuffer.writeAsync(buffer, data, size);
        return true;
    }
    return false;
}


bool WgContext::allocateBufferIndex(WGPUBuffer& buffer, const uint32_t* data, uint64_t size)
{
    if ((buffer) && (wgpuBufferGetSize(buffer) >= size)) {
        //wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
        stagedBuffer.writeAsync(buffer, data, size);
    } else {
        releaseBuffer(buffer);
        const WGPUBufferDescriptor bufferDesc {
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
            .size = size > WG_INDEX_BUFFER_MIN_SIZE ? size : WG_INDEX_BUFFER_MIN_SIZE
        };
        buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        //wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
        stagedBuffer.writeAsync(buffer, data, size);
        return true;
    }
    return false;
}


bool WgContext::allocateBufferIndexFan(uint64_t vertexCount)
{
    uint64_t indexCount = (vertexCount - 2) * 3;
    if ((!bufferIndexFan) || (wgpuBufferGetSize(bufferIndexFan) < indexCount * sizeof(uint32_t))) {
        tvg::Array<uint32_t> indexes(indexCount);
        for (size_t i = 0; i < vertexCount - 2; i++) {
            indexes.push(0);
            indexes.push(i + 1);
            indexes.push(i + 2);
        }
        releaseBuffer(bufferIndexFan);
        WGPUBufferDescriptor bufferDesc{
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
            .size = indexCount * sizeof(uint32_t)
        };
        bufferIndexFan = wgpuDeviceCreateBuffer(device, &bufferDesc);
        wgpuQueueWriteBuffer(queue, bufferIndexFan, 0, &indexes[0], indexCount * sizeof(uint32_t));
        return true;
    }
    return false;
}


void WgContext::releaseBuffer(WGPUBuffer& buffer)
{
    if (buffer) { 
        wgpuBufferDestroy(buffer);
        wgpuBufferRelease(buffer);
        buffer = nullptr;
    }
}

void WgContext::releaseQueue(WGPUQueue& queue)
{
    if (queue) {
        wgpuQueueRelease(queue);
        queue = nullptr;
    }
}