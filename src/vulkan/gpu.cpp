#include "hg/gpu.hpp"

#include "vulkan_internal.hpp"
#include "hg/utility.hpp"

#include <cmath>

namespace hg {
using namespace vulkan;

void gpuWaitIdle()
{
    vkQueueWaitIdle(vk.queue);
}

GpuBuffer GpuBuffer::create(u64 size, GpuBufferUsageFlags usageFlags, GpuMemoryUsage memoryUsage)
{
    HG_ASSERT(size > 0);
    HG_ASSERT(usageFlags != 0);

    GpuBuffer buf{};
    buf.data = makeUnique<GpuBufferData>();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = gpuBufferUsageToVk(usageFlags);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = gpuMemoryUsageToVma(memoryUsage);

    VkResult result = vmaCreateBuffer(
        vk.vma,
        &bufferInfo,
        &allocInfo,
        &buf.data->buffer,
        &buf.data->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        HG_PANIC("Could not create VkBuffer: %s\n", vkResultToStr(result));

    buf.data->size = size;

    if (usageFlags & GpuBufferUsage_uniformBuffer)
        buf.data->uniformDesc = createBufferDescriptor(DescriptorType_uniformBuffer, buf, 0, size);

    if (usageFlags & GpuBufferUsage_storageBuffer)
        buf.data->storageDesc = createBufferDescriptor(DescriptorType_storageBuffer, buf, 0, size);

    buf.data->usage = usageFlags;

    if (memoryUsage == GpuMemoryUsage_frequentUpdate)
    {
        VkMemoryPropertyFlags memPropFlags;
        vmaGetAllocationMemoryProperties(vk.vma, buf.data->alloc, &memPropFlags);
        buf.data->access = memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            ? GpuMemoryHostAccess_write
            : GpuMemoryHostAccess_none;
    } else {
        buf.data->access = gpuMemoryUsageToHostAccess(memoryUsage);
    }

    return buf;
}

GpuBufferData::~GpuBufferData()
{
    if (buffer != nullptr)
    {
        descriptorDestroy(storageDesc, DescriptorType_storageBuffer);
        descriptorDestroy(uniformDesc, DescriptorType_uniformBuffer);
        vmaDestroyBuffer(vk.vma, buffer, alloc);
    }
}

GpuBuffer::GpuBuffer() noexcept
    : data{nullptr}
{}

GpuBuffer::~GpuBuffer() noexcept = default;
GpuBuffer::GpuBuffer(GpuBuffer&&) noexcept = default;
GpuBuffer& GpuBuffer::operator=(GpuBuffer&&) noexcept = default;

u32 GpuBuffer::uniformDescriptor() const
{
    HG_ASSERT(data->usage & GpuBufferUsage_uniformBuffer);
    GpuDescriptor desc = data->uniformDesc;
    HG_ASSERT(vk.descriptorPools[DescriptorType_uniformBuffer].alive(desc));
    return desc.idx();
}

u32 GpuBuffer::storageDescriptor() const
{
    HG_ASSERT(data->usage & GpuBufferUsage_storageBuffer);
    GpuDescriptor desc = data->storageDesc;
    HG_ASSERT(vk.descriptorPools[DescriptorType_storageBuffer].alive(desc));
    return desc.idx();
}

void GpuBuffer::write(const void* src, u64 offset, u64 size)
{
    if (size == 0)
        return;

    HG_ASSERT(src != nullptr);

    if (data->access & GpuMemoryHostAccess_write)
    {
        VkResult result = vmaCopyMemoryToAllocation(vk.vma, src, data->alloc, offset, size);
        if (result != VK_SUCCESS)
            HG_PANIC("Could not write gpu buffer: %s\n", vkResultToStr(result));
        return;
    }

    GpuBuffer stage = GpuBuffer::create(size, GpuBufferUsage_transferSrc, GpuMemoryUsage_stagingWrite);
    stage.write(src, 0, size);

    GpuCmd* cmd = gpuCmdBegin();

    VkBufferCopy region{};
    region.dstOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(reinterpret_cast<VkCommandBuffer>(cmd), stage.data->buffer, data->buffer, 1, &region);

    gpuCmdEnd(cmd);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferWrite;
}

void GpuBuffer::read(void* dst, u64 offset, u64 size)
{
    if (size == 0)
        return;

    HG_ASSERT(dst != nullptr);

    if (data->access & GpuMemoryHostAccess_read)
    {
        VkResult result = vmaCopyAllocationToMemory(vk.vma, data->alloc, offset, dst, size);
        if (result != VK_SUCCESS)
            HG_PANIC("Could not read gpu buffer: %s\n", vkResultToStr(result));
        return;
    }

    GpuBuffer stage = GpuBuffer::create(size, GpuBufferUsage_transferDst, GpuMemoryUsage_stagingRead);

    GpuCmd* cmd = gpuCmdBegin();

    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = size;

    vkCmdCopyBuffer(reinterpret_cast<VkCommandBuffer>(cmd), data->buffer, stage.data->buffer, 1, &region);

    gpuCmdEnd(cmd);

    stage.read(dst, 0, size);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferRead;
}

GpuImage GpuImage::create(u32 width, u32 height, Format format, GpuImageUsageFlags usage)
{
    GpuImageCreateInfo create{};
    create.width = width;
    create.height = height;
    create.format = format;
    create.usage = usage;
    return GpuImage::createEx(create);
}

GpuImage GpuImage::createEx(const GpuImageCreateInfo& create)
{
    HG_ASSERT(create.format != Format_undefined);
    HG_ASSERT(create.usage != 0);

    GpuImage img{};
    img.data = makeUnique<GpuImageData>();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = gpuImageConfigFlagsToVk(create.flags);
    imageInfo.imageType = imageDimensionsToVkImage(create.dimensions);
    imageInfo.format = formatToVk(create.format);
    imageInfo.extent = {create.width, create.height, create.dimensions >= 3 ? create.depth : 1u};
    imageInfo.mipLevels = static_cast<u32>(create.mipLevels);
    imageInfo.arrayLayers = static_cast<u32>(create.arrayLayers);
    imageInfo.samples = countToMsaaSampleBits(create.msaaSamples);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = gpuImageUsageToVk(create.usage);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaCreateImage(
        vk.vma,
        &imageInfo,
        &allocInfo,
        &img.data->image,
        &img.data->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        HG_PANIC("Could not create VkImage: %s\n", vkResultToStr(result));

    img.data->usage = create.usage;
    img.data->format = create.format;
    img.data->width = create.width;
    img.data->height = create.height;
    img.data->depth = create.dimensions >= 3 ? create.depth : 1;
    img.data->dimensions = static_cast<u8>(create.dimensions);
    img.data->mipLevels = static_cast<u8>(create.mipLevels);
    img.data->arrayLayers = static_cast<u8>(create.arrayLayers);
    img.data->msaaSamples = static_cast<u8>(create.msaaSamples);

    return img;
}

GpuImageData::~GpuImageData()
{
    if (image != nullptr)
    {
        vmaDestroyImage(vk.vma, image, alloc);
    }
}

GpuImage::GpuImage() noexcept
    : data{nullptr}
{}

GpuImage::~GpuImage() noexcept = default;
GpuImage::GpuImage(GpuImage&&) noexcept = default;
GpuImage& GpuImage::operator=(GpuImage&&) noexcept = default;

u32 GpuImage::width() const
{
    return data->width;
}

u32 GpuImage::height() const
{
    return data->height;
}

GpuView GpuView::create(
    GpuImage& image,
    GpuAspectFlags aspectFlags,
    GpuFilter filter)
{
    GpuViewCreateInfo config{};
    config.image = &image;
    config.baseMipLevel = 0;
    config.levelCount = image.data->mipLevels;
    config.baseArrayLayer = 0;
    config.layerCount = image.data->arrayLayers;
    config.aspectFlags = aspectFlags;
    config.type = imageDimensionsToHgView(image.data->dimensions);
    config.filter = filter;
    config.edgeMode = GpuSamplerEdgeMode_repeat;
    config.border = GpuSamplerBorder_floatTransparentBlack;
    return GpuView::createEx(config);
}

GpuView GpuView::createEx(const GpuViewCreateInfo& config)
{
    HG_ASSERT(config.aspectFlags != 0);

    GpuView view{};
    view.data = makeUnique<GpuViewData>();

    GpuImageData* image = config.image->data;

    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image->image;
    info.viewType = gpuViewTypeToVk(config.type);
    info.format = formatToVk(image->format);
    info.subresourceRange.aspectMask = gpuAspectToVk(config.aspectFlags);
    info.subresourceRange.baseMipLevel = config.baseMipLevel;
    info.subresourceRange.levelCount = config.levelCount;
    info.subresourceRange.baseArrayLayer = config.baseArrayLayer;
    info.subresourceRange.layerCount = config.layerCount;

    [[maybe_unused]]
    VkResult result = vkCreateImageView(vk.device, &info, nullptr, &view.data->view);
    if (view.data->view == nullptr)
        HG_PANIC("Could not create VkImageView: %s\n", vkResultToStr(result));

    if (image->usage & GpuImageUsage_sampled)
    {
        view.data->sampler = samplerGet(config.filter, config.edgeMode, config.border);
        view.data->samplerDesc = createImageDescriptor(
            DescriptorType_combinedImageSampler,
            view,
            GpuLayout_shaderReadOnly);
    }

    if (image->usage & GpuImageUsage_storage)
    {
        view.data->storageDesc = createImageDescriptor(
            DescriptorType_storageImage,
            view,
            GpuLayout_general);
    }

    view.data->image = image;
    view.data->type = config.type;
    view.data->aspectFlags = config.aspectFlags;
    view.data->baseMipLevel = static_cast<u8>(config.baseMipLevel);
    view.data->levelCount = static_cast<u8>(config.levelCount);
    view.data->baseArrayLayer = static_cast<u8>(config.baseArrayLayer);
    view.data->layerCount = static_cast<u8>(config.layerCount);

    return view;
}

GpuViewData::~GpuViewData()
{
    if (view != nullptr)
    {
        descriptorDestroy(storageDesc, DescriptorType_storageImage);
        descriptorDestroy(samplerDesc, DescriptorType_combinedImageSampler);
        vkDestroyImageView(vk.device, view, nullptr);
    }
}

GpuView::GpuView() noexcept
    : data{nullptr}
{}

GpuView::~GpuView() noexcept = default;
GpuView::GpuView(GpuView&&) noexcept = default;
GpuView& GpuView::operator=(GpuView&&) noexcept = default;

u32 GpuView::width() const
{
    return data->image->width;
}

u32 GpuView::height() const
{
    return data->image->height;
}

u32 GpuView::samplerDescriptor() const
{
    HG_ASSERT(data->image->usage & GpuImageUsage_sampled);
    GpuDescriptor desc = data->samplerDesc;
    HG_ASSERT(vk.descriptorPools[DescriptorType_combinedImageSampler].alive(desc));
    return desc.idx();
}

u32 GpuView::storageDescriptor() const
{
    HG_ASSERT(data->image->usage & GpuImageUsage_storage);
    GpuDescriptor desc = data->storageDesc;
    HG_ASSERT(vk.descriptorPools[DescriptorType_storageImage].alive(desc));
    return desc.idx();
}

void GpuView::write(const void* src)
{
    HG_ASSERT(src != nullptr);

    u64 size = data->image->width
             * data->image->height
             * data->image->depth
             * formatToSize(data->image->format);

    GpuBuffer stage = GpuBuffer::create(size, GpuBufferUsage_transferSrc, GpuMemoryUsage_stagingWrite);
    stage.write(src, 0, size);

    GpuCmd* cmd = gpuCmdBegin();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = data->image->image;
    transferBarrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    transferBarrier.subresourceRange.baseMipLevel = data->baseMipLevel;
    transferBarrier.subresourceRange.levelCount = data->levelCount;
    transferBarrier.subresourceRange.baseArrayLayer = data->baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = data->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = gpuAspectToVk(data->aspectFlags);
    region.imageSubresource.mipLevel = data->baseMipLevel;
    region.imageSubresource.baseArrayLayer = data->baseArrayLayer;
    region.imageSubresource.layerCount = data->layerCount;
    region.imageExtent = {data->image->width, data->image->height, data->image->depth};

    vkCmdCopyBufferToImage(
        reinterpret_cast<VkCommandBuffer>(cmd),
        stage.data->buffer,
        data->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    gpuCmdEnd(cmd);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferWrite;
    data->lastLayout = GpuLayout_transferDst;
}

void GpuView::writeCubemap(const void* src)
{
    HG_ASSERT(data->image->depth == 1);
    HG_ASSERT(data->baseArrayLayer == 0);
    HG_ASSERT(data->layerCount >= 6);
    HG_ASSERT(src != nullptr);

    u64 size = data->image->width * data->image->height * formatToSize(data->image->format);

    GpuBuffer buffer = GpuBuffer::create(size * 4 * 3, GpuBufferUsage_transferSrc, GpuMemoryUsage_stagingWrite);
    buffer.write(src, 0, size * 4 * 3);

    GpuImage stage = GpuImage::create(
        data->image->width * 4,
        data->image->height * 3,
        data->image->format,
        GpuImageUsage_transferDst | GpuImageUsage_transferSrc);

    GpuCmd* cmd = gpuCmdBegin();

    VkImageMemoryBarrier2 stageBarrier{};
    stageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    stageBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    stageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    stageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    stageBarrier.image = stage.data->image;
    stageBarrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    stageBarrier.subresourceRange.baseMipLevel = 0;
    stageBarrier.subresourceRange.levelCount = 1;
    stageBarrier.subresourceRange.baseArrayLayer = 0;
    stageBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo stageDep{};
    stageDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    stageDep.imageMemoryBarrierCount = 1;
    stageDep.pImageMemoryBarriers = &stageBarrier;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &stageDep);

    VkBufferImageCopy stageRegion{};
    stageRegion.imageSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    stageRegion.imageExtent = {data->image->width * 4, data->image->height * 3, 1};

    vkCmdCopyBufferToImage(
        reinterpret_cast<VkCommandBuffer>(cmd),
        buffer.data->buffer,
        stage.data->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &stageRegion);

    VkImageMemoryBarrier2 transferBarriers[2]{};

    transferBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarriers[0].image = stage.data->image;
    transferBarriers[0].subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    transferBarriers[0].subresourceRange.baseMipLevel = 0;
    transferBarriers[0].subresourceRange.levelCount = 1;
    transferBarriers[0].subresourceRange.baseArrayLayer = 0;
    transferBarriers[0].subresourceRange.layerCount = 1;

    transferBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[1].image = data->image->image;
    transferBarriers[1].subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    transferBarriers[1].subresourceRange.baseMipLevel = data->baseMipLevel;
    transferBarriers[1].subresourceRange.levelCount = data->levelCount;
    transferBarriers[1].subresourceRange.baseArrayLayer = data->baseArrayLayer;
    transferBarriers[1].subresourceRange.layerCount = data->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = static_cast<u32>(hg::size(transferBarriers));
    transferDep.pImageMemoryBarriers = transferBarriers;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &transferDep);

    VkImageCopy regions[6]{};

    regions[0].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[0].srcOffset = {static_cast<int>(data->image->width) * 2, static_cast<int>(data->image->height) * 1, 0};
    regions[0].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 0, 1};
    regions[0].dstOffset = {};
    regions[0].extent = {data->image->width, data->image->height, 1};

    regions[1].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[1].srcOffset = {static_cast<int>(data->image->width) * 0, static_cast<int>(data->image->height) * 1, 0};
    regions[1].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 1, 1};
    regions[1].dstOffset = {};
    regions[1].extent = {data->image->width, data->image->height, 1};

    regions[2].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[2].srcOffset = {static_cast<int>(data->image->width) * 1, static_cast<int>(data->image->height) * 2, 0};
    regions[2].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 2, 1};
    regions[2].dstOffset = {};
    regions[2].extent = {data->image->width, data->image->height, 1};

    regions[3].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[3].srcOffset = {static_cast<int>(data->image->width) * 1, static_cast<int>(data->image->height) * 0, 0};
    regions[3].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 3, 1};
    regions[3].dstOffset = {};
    regions[3].extent = {data->image->width, data->image->height, 1};

    regions[4].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[4].srcOffset = {static_cast<int>(data->image->width) * 1, static_cast<int>(data->image->height) * 1, 0};
    regions[4].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 4, 1};
    regions[4].dstOffset = {};
    regions[4].extent = {data->image->width, data->image->height, 1};

    regions[5].srcSubresource = {gpuAspectToVk(data->aspectFlags), 0, 0, 1};
    regions[5].srcOffset = {static_cast<int>(data->image->width) * 3, static_cast<int>(data->image->height) * 1, 0};
    regions[5].dstSubresource = {gpuAspectToVk(data->aspectFlags), data->baseMipLevel, 5, 1};
    regions[5].dstOffset = {};
    regions[5].extent = {data->image->width, data->image->height, 1};

    vkCmdCopyImage(
        reinterpret_cast<VkCommandBuffer>(cmd),
        stage.data->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        data->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<u32>(hg::size(regions)),
        regions);

    gpuCmdEnd(cmd);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferWrite;
    data->lastLayout = GpuLayout_transferDst;
}

void GpuView::read(void* dst)
{
    HG_ASSERT(data->lastLayout != GpuLayout_undefined);
    HG_ASSERT(data != nullptr);

    u32 mipW = data->image->width >> data->baseMipLevel;
    u32 mipH = data->image->height >> data->baseMipLevel;
    u32 mipD = data->image->depth >> data->baseMipLevel;
    if (mipW < 1) mipW = 1;
    if (mipH < 1) mipH = 1;
    if (mipD < 1) mipD = 1;

    u64 size = mipW * mipH * mipD * formatToSize(data->image->format);

    GpuBuffer stage = GpuBuffer::create(size, GpuBufferUsage_transferDst, GpuMemoryUsage_stagingRead);

    GpuCmd* cmd = gpuCmdBegin();

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.srcStageMask = gpuStageToVk(data->lastStage);
    transferBarrier.srcAccessMask = gpuAccessToVk(data->lastAccess);
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.oldLayout = gpuLayoutToVk(data->lastLayout);
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarrier.image = data->image->image;
    transferBarrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    transferBarrier.subresourceRange.baseMipLevel = data->baseMipLevel;
    transferBarrier.subresourceRange.levelCount = data->levelCount;
    transferBarrier.subresourceRange.baseArrayLayer = data->baseArrayLayer;
    transferBarrier.subresourceRange.layerCount = data->layerCount;

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = gpuAspectToVk(data->aspectFlags);
    region.imageSubresource.mipLevel = data->baseMipLevel;
    region.imageSubresource.baseArrayLayer = data->baseArrayLayer;
    region.imageSubresource.layerCount = data->layerCount;
    region.imageExtent = {mipW, mipH, mipD};

    vkCmdCopyImageToBuffer(
        reinterpret_cast<VkCommandBuffer>(cmd),
        data->image->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        stage.data->buffer,
        1,
        &region);

    gpuCmdEnd(cmd);

    stage.read(dst, 0, size);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferRead;
    data->lastLayout = GpuLayout_transferSrc;
}

void GpuView::genMipmaps()
{
    HG_ASSERT(data->lastLayout != GpuLayout_undefined);
    if (data->levelCount == 1)
        return;

    GpuCmd* cmd = gpuCmdBegin();

    VkOffset3D mipOffset{};
    mipOffset.x = static_cast<i32>(data->image->width);
    mipOffset.y = static_cast<i32>(data->image->height);
    mipOffset.z = static_cast<i32>(data->image->depth);

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = gpuLayoutToVk(data->lastLayout);
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = data->image->image;
    barrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);

    for (u32 level = 0; level < static_cast<u32>(data->levelCount) - 1; ++level)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);

        VkImageBlit blit{};
        blit.srcSubresource.aspectMask = gpuAspectToVk(data->aspectFlags);
        blit.srcSubresource.mipLevel = level;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[1] = mipOffset;
        if (mipOffset.x > 1)
            mipOffset.x /= 2;
        if (mipOffset.y > 1)
            mipOffset.y /= 2;
        if (mipOffset.z > 1)
            mipOffset.z /= 2;
        blit.dstSubresource.aspectMask = gpuAspectToVk(data->aspectFlags);
        blit.dstSubresource.mipLevel = level + 1;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[1] = mipOffset;

        vkCmdBlitImage(
            reinterpret_cast<VkCommandBuffer>(cmd),
            data->image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            data->image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.aspectMask = gpuAspectToVk(data->aspectFlags);
        barrier.subresourceRange.baseMipLevel = level + 1;

        vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);
    }

    gpuCmdEnd(cmd);

    data->lastStage = GpuStage_transfer;
    data->lastAccess = GpuAccess_transferRead;
    data->lastLayout = GpuLayout_transferSrc;
}

u32 getMaxMipmaps(u32 width, u32 height, u32 depth)
{
    u32 max = width > height ? width : height;
    max = max > depth ? max : depth;
    return max == 0 ? 0 : static_cast<u32>(std::log2(static_cast<f32>(max))) + 1;
}

static VkShaderModule createShaderModule(const void* spirvCode, u64 codeSize)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = codeSize;
    info.pCode = (const u32*)spirvCode;

    VkShaderModule shader = nullptr;
    [[maybe_unused]] VkResult result = vkCreateShaderModule(vk.device, &info, nullptr, &shader);
    if (shader == nullptr)
        HG_PANIC("Could not create VkShaderModule: %s\n", vkResultToStr(result));

    return shader;
}

GpuPipeline GpuPipeline::graphics(const GpuGraphicsPipelineCreateInfo& config)
{
    HG_ASSERT(config.vertexShader.data != nullptr);
    HG_ASSERT(config.fragmentShader.data != nullptr);
    if (config.colorAttachmentFormats.count > 0)
        HG_ASSERT(config.colorAttachmentFormats.data != nullptr);

    GpuPipeline pipe{};
    pipe.data = makeUnique<GpuPipelineData>();

    ArenaScope scratch = getScratch();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &vk.bindlessLayout;
    VkPushConstantRange pushRange{VK_SHADER_STAGE_ALL, 0, config.pushConstantSize};
    if (config.pushConstantSize > 0)
    {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
    }

    [[maybe_unused]]
    VkResult layoutResult = vkCreatePipelineLayout(vk.device, &layoutInfo, nullptr, &pipe.data->layout);
    if (pipe.data->layout == nullptr)
        HG_PANIC("Could not create VkPipelineLayout: %s\n", vkResultToStr(layoutResult));

    VkShaderModule vertexShader = createShaderModule(config.vertexShader.data, config.vertexShader.count);
    VkShaderModule fragmentShader = createShaderModule(config.fragmentShader.data, config.fragmentShader.count);
    HG_DEFER(vkDestroyShaderModule(vk.device, vertexShader, nullptr));
    HG_DEFER(vkDestroyShaderModule(vk.device, fragmentShader, nullptr));

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShader;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShader;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = gpuTopologyToVk(config.topology);
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tessellationState{};
    tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationState.patchControlPoints = config.tesselationPatchControlPoints;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = gpuPolygonModeToVk(config.polygonMode);
    rasterizationState.cullMode = gpuCullModeToVk(config.cullMode);
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = countToMsaaSampleBits(config.multisampleCount);
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.minSampleShading = 1.0f;
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = config.enableDepthRead;
    depthStencilState.depthWriteEnable = config.enableDepthWrite;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = config.enableDepthRead;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = {};
    depthStencilState.back = {};
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    ArrayTemp<VkPipelineColorBlendAttachmentState> colorBlendAttachments{
        scratch, 0, static_cast<u32>(config.colorAttachmentFormats.count)};

    for (u32 i = 0; i < config.colorAttachmentFormats.count; ++i)
    {
        VkPipelineColorBlendAttachmentState& attachment = colorBlendAttachments.push();
        attachment.blendEnable = config.colorBlendEnables.data != nullptr
            ? config.colorBlendEnables[i]
            : VK_FALSE;
        attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.colorBlendOp = VK_BLEND_OP_ADD;
        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        attachment.colorWriteMask
            = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = static_cast<u32>(colorBlendAttachments.count);
    colorBlendState.pAttachments = colorBlendAttachments.vals;
    for (float& blendConstant : colorBlendState.blendConstants)
    {
        blendConstant = 1.0f;
    }

    VkDynamicState dynamicStates[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(size(dynamicStates));
    dynamicState.pDynamicStates = dynamicStates;

    ArrayTemp<VkFormat> colorFormats{scratch, 0, static_cast<u32>(config.colorAttachmentFormats.count)};
    for (u32 i = 0; i < config.colorAttachmentFormats.count; ++i)
    {
        colorFormats.push(formatToVk(config.colorAttachmentFormats[i]));
    }
    VkFormat depthFormat = formatToVk(config.depthAttachmentFormat);
    VkFormat stencilFormat = formatToVk(config.stencilAttachmentFormat);

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = static_cast<u32>(colorFormats.count);
    renderingInfo.pColorAttachmentFormats = colorFormats.vals;
    renderingInfo.depthAttachmentFormat = depthFormat;
    renderingInfo.stencilAttachmentFormat = stencilFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<u32>(size(shaderStages));
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineInfo.pTessellationState = &tessellationState;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipe.data->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    [[maybe_unused]]
    VkResult pipelineResult = vkCreateGraphicsPipelines(
        vk.device, nullptr, 1, &pipelineInfo, nullptr, &pipe.data->pipeline);
    if (pipe.data->pipeline == nullptr)
        HG_PANIC("Failed to create Vulkan graphics pipeline: %s\n", vkResultToStr(pipelineResult));

    pipe.data->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    return pipe;
}

GpuPipeline GpuPipeline::compute(Span<const u8> shaderCode, u32 pushSize)
{
    HG_ASSERT(shaderCode.data != nullptr);
    HG_ASSERT(shaderCode.count > 0);

    GpuPipeline pipe{};
    pipe.data = makeUnique<GpuPipelineData>();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &vk.bindlessLayout;

    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, pushSize};
    layoutInfo.pushConstantRangeCount = pushSize > 0 ? 1 : 0;
    layoutInfo.pPushConstantRanges = pushSize > 0 ? &push : nullptr;

    [[maybe_unused]]
    VkResult layoutResult = vkCreatePipelineLayout(vk.device, &layoutInfo, nullptr, &pipe.data->layout);
    if (pipe.data->layout == nullptr)
        HG_PANIC("Could not create VkPipelineLayout: %s\n", vkResultToStr(layoutResult));

    VkShaderModule computeShader = createShaderModule(shaderCode.data, static_cast<u32>(shaderCode.count));
    HG_DEFER(vkDestroyShaderModule(vk.device, computeShader, nullptr));

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = computeShader;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = pipe.data->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    [[maybe_unused]]
    VkResult pipelineResult = vkCreateComputePipelines(
        vk.device, nullptr, 1, &pipelineInfo, nullptr, &pipe.data->pipeline);
    if (pipe.data->pipeline == nullptr)
        HG_PANIC("Failed to create Vulkan compute pipeline: %s\n", vkResultToStr(pipelineResult));

    pipe.data->bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

    return pipe;
}

GpuPipelineData::~GpuPipelineData()
{
    if (pipeline != nullptr)
    {
        vkDestroyPipeline(vk.device, pipeline, nullptr);
        vkDestroyPipelineLayout(vk.device, layout, nullptr);
    }
}

GpuPipeline::GpuPipeline() noexcept
    : data{nullptr}
{}

GpuPipeline::~GpuPipeline() noexcept = default;
GpuPipeline::GpuPipeline(GpuPipeline&&) noexcept = default;
GpuPipeline& GpuPipeline::operator=(GpuPipeline&&) noexcept = default;

GpuCmd* gpuCmdBegin()
{
    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = vk.cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(vk.device, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return reinterpret_cast<GpuCmd*>(cmd);
}

void gpuCmdEnd(GpuCmd* cmd)
{
    HG_ASSERT(cmd != nullptr);
    vkEndCommandBuffer(reinterpret_cast<VkCommandBuffer>(cmd));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;
    vkCreateFence(vk.device, &fenceInfo, nullptr, &fence);
    HG_DEFER(vkDestroyFence(vk.device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = reinterpret_cast<VkCommandBuffer*>(&cmd);

    vkQueueSubmit(vk.queue, 1, &submit, fence);
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(vk.device, vk.cmdPool, 1, reinterpret_cast<VkCommandBuffer*>(&cmd));
}

void gpuBindPipeline(GpuCmd* cmd, const GpuPipeline& pipeline)
{
    vkCmdBindPipeline(
        reinterpret_cast<VkCommandBuffer>(cmd),
        pipeline.data->bindPoint,
        pipeline.data->pipeline);

    vkCmdBindDescriptorSets(
        reinterpret_cast<VkCommandBuffer>(cmd),
        pipeline.data->bindPoint,
        pipeline.data->layout,
        0,
        1,
        &vk.bindlessSet,
        0,
        nullptr);
}

void gpuPushConstants(GpuCmd* cmd, const GpuPipeline& pipeline, void* push, u32 size)
{
    vkCmdPushConstants(
        reinterpret_cast<VkCommandBuffer>(cmd),
        pipeline.data->layout,
        pipeline.data->bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE
            ? VK_SHADER_STAGE_COMPUTE_BIT
            : VK_SHADER_STAGE_ALL,
        0,
        size,
        push);
}

void gpuDraw(GpuCmd* cmd, u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount)
{
    vkCmdDraw(reinterpret_cast<VkCommandBuffer>(cmd), vertexCount, instanceCount, vertexBegin, instanceBegin);
}

void gpuDispatch(GpuCmd* cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ)
{
    vkCmdDispatch(reinterpret_cast<VkCommandBuffer>(cmd), groupCountX, groupCountY, groupCountZ);
}

void gpuMemoryBarrier(
    GpuCmd* cmd,
    Span<const GpuBufferBarrier> bufferBarriers,
    Span<const GpuImageBarrier> imageBarriers)
{
    ArenaScope scratch = getScratch();

    ArrayTemp<VkBufferMemoryBarrier2> vkBufferBarriers{scratch, 0, static_cast<u32>(bufferBarriers.count)};
    ArrayTemp<VkImageMemoryBarrier2> vkImageBarriers{scratch, 0, static_cast<u32>(imageBarriers.count)};

    for (const GpuBufferBarrier& barrier : bufferBarriers)
    {
        VkBufferMemoryBarrier2& vkBarrier = vkBufferBarriers.push();

        vkBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBarrier.srcStageMask = gpuStageToVk(barrier.buffer->data->lastStage);
        vkBarrier.srcAccessMask = gpuAccessToVk(barrier.buffer->data->lastAccess);
        vkBarrier.dstStageMask = gpuStageToVk(barrier.nextStage);
        vkBarrier.dstAccessMask = gpuAccessToVk(barrier.nextAccess);
        vkBarrier.buffer = barrier.buffer->data->buffer;
        vkBarrier.size = barrier.buffer->data->size;

        barrier.buffer->data->lastStage = barrier.nextStage;
        barrier.buffer->data->lastAccess = barrier.nextAccess;
    }

    for (const GpuImageBarrier& barrier : imageBarriers)
    {
        VkImageMemoryBarrier2& vkBarrier = vkImageBarriers.push();

        vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkBarrier.srcStageMask = gpuStageToVk(barrier.image->data->lastStage);
        vkBarrier.srcAccessMask = gpuAccessToVk(barrier.image->data->lastAccess);
        vkBarrier.dstStageMask = gpuStageToVk(barrier.nextStage);
        vkBarrier.dstAccessMask = gpuAccessToVk(barrier.nextAccess);
        vkBarrier.oldLayout = gpuLayoutToVk(barrier.image->data->lastLayout);
        vkBarrier.newLayout = gpuLayoutToVk(barrier.nextLayout);
        vkBarrier.image = barrier.image->data->image->image;
        vkBarrier.subresourceRange = {
            gpuAspectToVk(barrier.image->data->aspectFlags),
            barrier.image->data->baseMipLevel,
            barrier.image->data->levelCount,
            barrier.image->data->baseArrayLayer,
            barrier.image->data->layerCount,
        };

        barrier.image->data->lastStage = barrier.nextStage;
        barrier.image->data->lastAccess = barrier.nextAccess;
        barrier.image->data->lastLayout = barrier.nextLayout;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = static_cast<u32>(vkBufferBarriers.count);
    dep.pBufferMemoryBarriers = vkBufferBarriers.vals;
    dep.imageMemoryBarrierCount = static_cast<u32>(vkImageBarriers.count);
    dep.pImageMemoryBarriers = vkImageBarriers.vals;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);
}

void gpuComputePass(GpuCmd* cmd, const GpuComputePass& pass)
{
    ArenaScope scratch = getScratch();

    ArrayTemp<VkBufferMemoryBarrier2> bufferBarriers{scratch, 0, 32};
    ArrayTemp<VkImageMemoryBarrier2> imageBarriers{scratch, 0, 32};

    for (u32 i = 0; i < pass.uniformBuffers.count; ++i)
    {
        VkBufferMemoryBarrier2& barrier = bufferBarriers.push();
        GpuBufferData* buffer = pass.uniformBuffers[i]->data;

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(buffer->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        barrier.buffer = buffer->buffer;
        barrier.size = buffer->size;

        buffer->lastStage = GpuStage_computeShader;
        buffer->lastAccess = GpuAccess_uniformRead;
    }

    for (u32 i = 0; i < pass.storageBuffers.count; ++i)
    {
        VkBufferMemoryBarrier2& barrier = bufferBarriers.push();
        GpuBufferData* buffer = pass.storageBuffers[i]->data;

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(buffer->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.buffer = buffer->buffer;
        barrier.size = buffer->size;

        buffer->lastStage = GpuStage_computeShader;
        buffer->lastAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
    }

    for (u32 i = 0; i < pass.sampledImages.count; ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.push();
        GpuViewData* image = pass.sampledImages[i]->data;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(image->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_computeShader;
        image->lastAccess = GpuAccess_shaderRead;
        image->lastLayout = GpuLayout_shaderReadOnly;
    }

    for (u32 i = 0; i < pass.storageImages.count; ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.push();
        GpuViewData* image = pass.storageImages[i]->data;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(image->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_computeShader;
        image->lastAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        image->lastLayout = GpuLayout_general;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = static_cast<u32>(bufferBarriers.count);
    dep.pBufferMemoryBarriers = bufferBarriers.vals;
    dep.imageMemoryBarrierCount = static_cast<u32>(imageBarriers.count);
    dep.pImageMemoryBarriers = imageBarriers.vals;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);
}

void gpuBeginRenderPass(GpuCmd* cmd, const GpuRenderPass& pass)
{
    ArenaScope scratch = getScratch();

    ArrayTemp<VkBufferMemoryBarrier2> bufferBarriers{scratch, 0, 32};
    ArrayTemp<VkImageMemoryBarrier2> imageBarriers{scratch, 0, 32};

    for (u32 i = 0; i < pass.uniformBuffers.count; ++i)
    {
        VkBufferMemoryBarrier2& barrier = bufferBarriers.push();
        GpuBufferData* buffer = pass.uniformBuffers[i]->data;

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(buffer->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        barrier.buffer = buffer->buffer;
        barrier.size = buffer->size;

        buffer->lastStage = GpuStage_vertexShader | GpuStage_fragmentShader;
        buffer->lastAccess = GpuAccess_uniformRead;
    }

    for (u32 i = 0; i < pass.storageBuffers.count; ++i)
    {
        VkBufferMemoryBarrier2& barrier = bufferBarriers.push();
        GpuBufferData* buffer = pass.storageBuffers[i]->data;

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(buffer->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(buffer->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.buffer = buffer->buffer;
        barrier.size = buffer->size;

        buffer->lastStage = GpuStage_vertexShader | GpuStage_fragmentShader;
        buffer->lastAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
    }

    for (u32 i = 0; i < pass.sampledImages.count; ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.push();
        GpuViewData* image = pass.sampledImages[i]->data;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(image->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_fragmentShader;
        image->lastAccess = GpuAccess_shaderRead;
        image->lastLayout = GpuLayout_shaderReadOnly;
    }

    for (u32 i = 0; i < pass.storageImages.count; ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.push();
        GpuViewData* image = pass.storageImages[i]->data;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(image->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_fragmentShader;
        image->lastAccess = GpuAccess_shaderRead | GpuAccess_shaderWrite;
        image->lastLayout = GpuLayout_general;
    }

    for (u32 i = 0; i < pass.colorAttachments.count; ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.push();
        GpuViewData* image = pass.colorAttachments[i].image->data;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(image->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (pass.colorAttachments[i].loadOp == GpuLoadOp_load)
            barrier.oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_colorAttachmentOutput;
        image->lastAccess = GpuAccess_colorAttachmentWrite;
        image->lastLayout = GpuLayout_colorAttachment;
    }

    if (pass.depthAttachment != nullptr)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.push();
        GpuViewData* image = pass.depthAttachment->image->data;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(image->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                              | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (pass.depthAttachment->loadOp == GpuLoadOp_load)
            barrier.oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_earlyFragmentTests | GpuStage_lateFragmentTests;
        image->lastAccess = GpuAccess_depthStencilAttachmentRead | GpuAccess_depthStencilAttachmentWrite;
        image->lastLayout = GpuLayout_depthStencilAttachment;
    }

    if (pass.stencilAttachment != nullptr)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.push();
        GpuViewData* image = pass.stencilAttachment->image->data;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStageToVk(image->lastStage);
        barrier.srcAccessMask = gpuAccessToVk(image->lastAccess);
        barrier.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                              | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (pass.stencilAttachment->loadOp == GpuLoadOp_load)
            barrier.oldLayout = gpuLayoutToVk(image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuAspectToVk(image->aspectFlags),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->layerCount,
        };

        image->lastStage = GpuStage_earlyFragmentTests | GpuStage_lateFragmentTests;
        image->lastAccess = GpuAccess_depthStencilAttachmentRead | GpuAccess_depthStencilAttachmentWrite;
        image->lastLayout = GpuLayout_depthStencilAttachment;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = static_cast<u32>(bufferBarriers.count);
    dep.pBufferMemoryBarriers = bufferBarriers.vals;
    dep.imageMemoryBarrierCount = static_cast<u32>(imageBarriers.count);
    dep.pImageMemoryBarriers = imageBarriers.vals;

    vkCmdPipelineBarrier2(reinterpret_cast<VkCommandBuffer>(cmd), &dep);

    ArrayTemp<VkRenderingAttachmentInfo> colorAttachments{
        scratch, 0, static_cast<u32>(pass.colorAttachments.count)};

    for (u32 i = 0; i < pass.colorAttachments.count; ++i)
    {
        VkRenderingAttachmentInfo& attachment = colorAttachments.push();
        attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachment.imageView = pass.colorAttachments[i].image->data->view;
        attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.loadOp = gpuLoadOpToVk(pass.colorAttachments[i].loadOp);
        attachment.storeOp = gpuStoreOpToVk(pass.colorAttachments[i].storeOp);
        memcpy(&attachment.clearValue, &pass.colorAttachments[i].clearValue, sizeof(VkClearValue));
    }

    VkRenderingAttachmentInfo depthAttachment{};
    if (pass.depthAttachment != nullptr)
    {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = pass.depthAttachment->image->data->view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = gpuLoadOpToVk(pass.depthAttachment->loadOp);
        depthAttachment.storeOp = gpuStoreOpToVk(pass.depthAttachment->storeOp);
        memcpy(&depthAttachment.clearValue, &pass.depthAttachment->clearValue, sizeof(VkClearValue));
    }

    VkRenderingAttachmentInfo stencilAttachment{};
    if (pass.stencilAttachment != nullptr)
    {
        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachment.imageView = pass.stencilAttachment->image->data->view;
        stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencilAttachment.loadOp = gpuLoadOpToVk(pass.stencilAttachment->loadOp);
        stencilAttachment.storeOp = gpuStoreOpToVk(pass.stencilAttachment->storeOp);
        memcpy(&stencilAttachment.clearValue, &pass.stencilAttachment->clearValue, sizeof(VkClearValue));
    }

    u32 width, height;
    if (pass.colorAttachments.count > 0)
    {
        width = pass.colorAttachments[0].image->data->image->width;
        height = pass.colorAttachments[0].image->data->image->height;
    }
    else if (pass.depthAttachment != nullptr)
    {
        width = pass.depthAttachment->image->data->image->width;
        height = pass.depthAttachment->image->data->image->height;
    }
    else
    {
        HG_ASSERT(pass.stencilAttachment != nullptr);
        width = pass.stencilAttachment->image->data->image->width;
        height = pass.stencilAttachment->image->data->image->height;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = pass.layerCount;
    renderingInfo.colorAttachmentCount = static_cast<u32>(pass.colorAttachments.count);
    renderingInfo.pColorAttachments = colorAttachments.vals;
    renderingInfo.pDepthAttachment = pass.depthAttachment != nullptr
        ? &depthAttachment
        : nullptr;
    renderingInfo.pStencilAttachment = pass.stencilAttachment != nullptr
        ? &stencilAttachment
        : nullptr;

    vkCmdBeginRendering(reinterpret_cast<VkCommandBuffer>(cmd), &renderingInfo);

    gpuSetViewport(cmd, 0, 0, static_cast<f32>(width), static_cast<f32>(height));
    gpuSetScissor(cmd, 0, 0, width, height);
}

void gpuEndRenderPass(GpuCmd* cmd)
{
    vkCmdEndRendering(reinterpret_cast<VkCommandBuffer>(cmd));
}

void gpuSetViewport(GpuCmd* cmd, f32 x, f32 y, f32 width, f32 height, f32 near, f32 far)
{
    VkViewport viewport{x, y, width, height, near, far};
    vkCmdSetViewport(reinterpret_cast<VkCommandBuffer>(cmd), 0, 1, &viewport);
}

void gpuSetScissor(GpuCmd* cmd, i32 x, i32 y, u32 width, u32 height)
{
    VkRect2D scissor{{x, y}, {width, height}};
    vkCmdSetScissor(reinterpret_cast<VkCommandBuffer>(cmd), 0, 1, &scissor);
}

GpuCmd* gpuBeginFrame(Span<Window*> windows)
{
    Frame* frame = &vk.frames[vk.currentFrame];

    vkWaitForFences(vk.device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &frame->fence);

    frame->swapchains.reset();
    for (u32 i = 0; i < windows.count; ++i)
    {
        internal::Swapchain& swap = *reinterpret_cast<internal::Swapchain*>(windows[i]->data.ptr);
        if (swap.data->swapchain == nullptr)
            continue;

        VkResult result = vkAcquireNextImageKHR(
            vk.device,
            swap.data->swapchain,
            UINT64_MAX,
            swap.data->imageAvailable[vk.currentFrame],
            nullptr,
            &swap.data->imageIdx);

        if (result == VK_SUCCESS)
        {
            frame->swapchains.push(&swap);
        }
        else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            swap.resize(swap.data->width, swap.data->height);
            swap.data->imageIdx = (u32)-1;
        }
        else
        {
            HG_PANIC("Could not acquire next image: %s\n", vkResultToStr(result));
        }
    }

    vkResetCommandPool(vk.device, frame->cmdPool, 0);

    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = frame->cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(vk.device, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    return reinterpret_cast<GpuCmd*>(cmd);
}

void gpuEndFrame(GpuCmd* cmd)
{
    HG_ASSERT(cmd != nullptr);

    ArenaScope scratch = getScratch();

    Frame* frame = &vk.frames[vk.currentFrame];

    ArrayTemp<GpuImageBarrier> presentBarriers = ArrayTemp<GpuImageBarrier>{
        scratch, 0, frame->swapchains.count};
    for (internal::Swapchain* swap : frame->swapchains)
    {
        GpuImageBarrier& barrier = presentBarriers.push();
        barrier.image = &swap->data->views[swap->data->imageIdx];
        barrier.nextLayout = GpuLayout_presentSrc;
    }
    gpuMemoryBarrier(cmd, {}, presentBarriers);

    vkEndCommandBuffer(reinterpret_cast<VkCommandBuffer>(cmd));

    ArrayTemp<VkPipelineStageFlags> waitStages{scratch, 0, frame->swapchains.count};
    ArrayTemp<VkSemaphore> imageAvailableSemaphores{scratch, 0, frame->swapchains.count};
    ArrayTemp<VkSemaphore> readyToPresentSemaphores{scratch, 0, frame->swapchains.count};

    ArrayTemp<VkSwapchainKHR> swapchains{scratch, 0, frame->swapchains.count};
    ArrayTemp<u32> imageIndices{scratch, 0, frame->swapchains.count};

    for (internal::Swapchain* swap : frame->swapchains)
    {
        waitStages.push(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        imageAvailableSemaphores.push(swap->data->imageAvailable[vk.currentFrame]);
        readyToPresentSemaphores.push(swap->data->readyToPresent[swap->data->imageIdx]);
        swapchains.push(swap->data->swapchain);
        imageIndices.push(swap->data->imageIdx);
        swap->data->imageIdx = (u32)-1;
    }

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = static_cast<u32>(imageAvailableSemaphores.count);
    submit.pWaitSemaphores = imageAvailableSemaphores.vals;
    submit.pWaitDstStageMask = waitStages.vals;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = reinterpret_cast<VkCommandBuffer*>(&cmd);
    submit.signalSemaphoreCount = static_cast<u32>(readyToPresentSemaphores.count);
    submit.pSignalSemaphores = readyToPresentSemaphores.vals;

    vkQueueSubmit(vk.queue, 1, &submit, frame->fence);

    if (frame->swapchains.count > 0)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = static_cast<u32>(readyToPresentSemaphores.count);
        presentInfo.pWaitSemaphores = readyToPresentSemaphores.vals;
        presentInfo.swapchainCount = static_cast<u32>(swapchains.count);
        presentInfo.pSwapchains = swapchains.vals;
        presentInfo.pImageIndices = imageIndices.vals;

        vkQueuePresentKHR(vk.queue, &presentInfo);
    }

    vk.currentFrame = (vk.currentFrame + 1) % vk.frameCount;
}

} // namespace hg
