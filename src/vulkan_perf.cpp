#include "hurdygurdy.hpp"

#include <random>

int main()
{
    hgInit();
    hgDefer(hgDeinit());

    HgArena* arena = hgGetScratch();
    HgArenaScope arenaScope{arena};

    u32 iterCount = 1024;
    u32 imageWidth = 1024;
    u32 imageHeight = 1024;

    HgFence fence;
    HgResource shaderCodeID = hgResourceID("build/vulkan_perf.comp.spv");
    hgLoadResource(&fence, 1, shaderCodeID, "build/vulkan_perf.comp.spv");
    hgDefer(hgUnloadResource(nullptr, 0, shaderCodeID));

    struct Push
    {
        u32 seed;
        u32 imageIdx;
    };

    VkPushConstantRange pushRange{VK_SHADER_STAGE_ALL, 0, sizeof(Push)};
    VkPipelineLayout pipelineLayout = hgCreatePipelineLayout(&pushRange);
    hgDefer(vkDestroyPipelineLayout(hgVkDevice, pipelineLayout, nullptr));

    hgWaitForFenceIndefinite(&fence);
    HgBinary* shaderCode = hgGetResource(shaderCodeID);
    VkPipeline pipeline = hgCreateComputePipeline(pipelineLayout, (u8*)shaderCode->data, shaderCode->size);
    hgDefer(vkDestroyPipeline(hgVkDevice, pipeline, nullptr));

    hgDebug("New Alloc (Manual)...\n");
    HgPerfStats newAllocManual{};
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgPerf perf = hgCreatePerf(arena, iterCount);

        for (u32 i = 0; i < iterCount; ++i)
        {
            u32 seed = std::random_device{}();

            hgBeginPerf(&perf);

            HgImage* image = hgAlloc<HgImage>(scratch, 1);
            image->type = VK_IMAGE_TYPE_2D;
            image->format = VK_FORMAT_R8G8B8A8_UNORM;
            image->width = imageWidth;
            image->height = imageHeight;
            image->depth = 1;
            image->mipLevels = 1;
            image->arrayLayers = 1;
            image->msaaSamples = VK_SAMPLE_COUNT_1_BIT;

            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageCreateInfo.extent = {imageWidth, imageHeight, 1};
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;

            vkCreateImage(hgVkDevice, &imageCreateInfo, NULL, &image->image);

            VkMemoryRequirements memReqs{};
            vkGetImageMemoryRequirements(hgVkDevice, image->image, &memReqs);

            u32 memIdx = hgFindVkMemoryTypeIndex(
                memReqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = memIdx;

            VkDeviceMemory imageAlloc = nullptr;
            vkAllocateMemory(hgVkDevice, &allocInfo, NULL, &imageAlloc);

            hgDefer(vkFreeMemory(hgVkDevice, imageAlloc, nullptr));
            hgDefer(vkDestroyImage(hgVkDevice, image->image, nullptr));

            vkBindImageMemory(hgVkDevice, image->image, imageAlloc, 0);

            HgImageView* view = hgCreateImageView(image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
            hgDefer(hgDestroyImageView(view));

            HgDescriptor desc = hgCreateDescriptor(HgDescriptorType_storageImage);
            hgDefer(hgDestroyDescriptor(desc));

            VkDescriptorImageInfo imageInfo{nullptr, view->view, VK_IMAGE_LAYOUT_GENERAL};
            hgUpdateDescriptor(desc, nullptr, &imageInfo);

            VkCommandBuffer cmd = hgBeginVkCmd();

            HgRenderer renderer = renderer.create(scratch, 4, 4);

            HgRenderPass pass{};
            pass.storageImages = &view;
            pass.storageImageCount = 1;

            renderer.prepareResources(cmd, &pass);

            hgBindComputePipeline(cmd, pipeline, pipelineLayout);

            Push push{};
            push.seed = seed;
            push.imageIdx = hgDescriptorIdx(desc);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(Push), &push);

            vkCmdDispatch(cmd, imageWidth / 16, imageHeight / 16, 1);

            hgEndVkCmd(cmd);

            hgEndPerf(&perf);
        }

        newAllocManual = hgAnalyzePerf(&perf);
    }

    // new allocation, VMA
    hgDebug("New Alloc (Vma)...\n");
    HgPerfStats newAllocVma{};
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgPerf perf = hgCreatePerf(arena, iterCount);

        for (u32 i = 0; i < iterCount; ++i)
        {
            u32 seed = std::random_device{}();

            hgBeginPerf(&perf);

            HgImage* image = hgCreateImage(
                imageWidth,
                imageHeight,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_STORAGE_BIT);
            hgDefer(hgDestroyImage(image));

            HgImageView* view = hgCreateImageView(image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
            hgDefer(hgDestroyImageView(view));

            HgDescriptor desc = hgCreateDescriptor(HgDescriptorType_storageImage);
            hgDefer(hgDestroyDescriptor(desc));

            VkDescriptorImageInfo imageInfo{nullptr, view->view, VK_IMAGE_LAYOUT_GENERAL};
            hgUpdateDescriptor(desc, nullptr, &imageInfo);

            VkCommandBuffer cmd = hgBeginVkCmd();

            HgRenderer renderer = renderer.create(scratch, 4, 4);

            HgRenderPass pass{};
            pass.storageImages = &view;
            pass.storageImageCount = 1;

            renderer.prepareResources(cmd, &pass);

            hgBindComputePipeline(cmd, pipeline, pipelineLayout);

            Push push{};
            push.seed = seed;
            push.imageIdx = hgDescriptorIdx(desc);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(Push), &push);

            vkCmdDispatch(cmd, imageWidth / 16, imageHeight / 16, 1);

            hgEndVkCmd(cmd);

            hgEndPerf(&perf);
        }

        newAllocVma = hgAnalyzePerf(&perf);
    }

    hgDebug("New Image (Manual)...\n");
    HgPerfStats newImageManual{};
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgImage* image = hgAlloc<HgImage>(scratch, 1);
        image->type = VK_IMAGE_TYPE_2D;
        image->format = VK_FORMAT_R8G8B8A8_UNORM;
        image->width = imageWidth;
        image->height = imageHeight;
        image->depth = 1;
        image->mipLevels = 1;
        image->arrayLayers = 1;
        image->msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.extent = {imageWidth, imageHeight, 1};
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;

        vkCreateImage(hgVkDevice, &imageCreateInfo, NULL, &image->image);

        VkMemoryRequirements memReqs{};
        vkGetImageMemoryRequirements(hgVkDevice, image->image, &memReqs);

        u32 memIdx = hgFindVkMemoryTypeIndex(
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = memIdx;

        VkDeviceMemory imageAlloc = nullptr;
        vkAllocateMemory(hgVkDevice, &allocInfo, NULL, &imageAlloc);
        hgDefer(vkFreeMemory(hgVkDevice, imageAlloc, nullptr));

        vkDestroyImage(hgVkDevice, image->image, nullptr);

        HgPerf perf = hgCreatePerf(arena, iterCount);

        for (u32 i = 0; i < iterCount; ++i)
        {
            u32 seed = std::random_device{}();

            hgBeginPerf(&perf);

            vkCreateImage(hgVkDevice, &imageCreateInfo, NULL, &image->image);
            hgDefer(vkDestroyImage(hgVkDevice, image->image, nullptr));

            vkBindImageMemory(hgVkDevice, image->image, imageAlloc, 0);

            HgImageView* view = hgCreateImageView(image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
            hgDefer(hgDestroyImageView(view));

            HgDescriptor desc = hgCreateDescriptor(HgDescriptorType_storageImage);
            hgDefer(hgDestroyDescriptor(desc));

            VkDescriptorImageInfo imageInfo{nullptr, view->view, VK_IMAGE_LAYOUT_GENERAL};
            hgUpdateDescriptor(desc, nullptr, &imageInfo);

            VkCommandBuffer cmd = hgBeginVkCmd();

            HgRenderer renderer = renderer.create(scratch, 4, 4);

            HgRenderPass pass{};
            pass.storageImages = &view;
            pass.storageImageCount = 1;

            renderer.prepareResources(cmd, &pass);

            hgBindComputePipeline(cmd, pipeline, pipelineLayout);

            Push push{};
            push.seed = seed;
            push.imageIdx = hgDescriptorIdx(desc);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(Push), &push);

            vkCmdDispatch(cmd, imageWidth / 16, imageHeight / 16, 1);

            hgEndVkCmd(cmd);

            hgEndPerf(&perf);
        }

        newImageManual = hgAnalyzePerf(&perf);
    }

    hgDebug("Same Image (manual)...\n");
    HgPerfStats sameImageManual{};
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgImage* image = hgAlloc<HgImage>(scratch, 1);
        image->type = VK_IMAGE_TYPE_2D;
        image->format = VK_FORMAT_R8G8B8A8_UNORM;
        image->width = imageWidth;
        image->height = imageHeight;
        image->depth = 1;
        image->mipLevels = 1;
        image->arrayLayers = 1;
        image->msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.extent = {imageWidth, imageHeight, 1};
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;

        vkCreateImage(hgVkDevice, &imageCreateInfo, NULL, &image->image);

        VkMemoryRequirements memReqs{};
        vkGetImageMemoryRequirements(hgVkDevice, image->image, &memReqs);

        u32 memIdx = hgFindVkMemoryTypeIndex(
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = memIdx;

        VkDeviceMemory imageAlloc = nullptr;
        vkAllocateMemory(hgVkDevice, &allocInfo, NULL, &imageAlloc);

        hgDefer(vkFreeMemory(hgVkDevice, imageAlloc, nullptr));
        hgDefer(vkDestroyImage(hgVkDevice, image->image, nullptr));

        vkBindImageMemory(hgVkDevice, image->image, imageAlloc, 0);

        HgImageView* view = hgCreateImageView(image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
        hgDefer(hgDestroyImageView(view));

        HgDescriptor desc = hgCreateDescriptor(HgDescriptorType_storageImage);
        hgDefer(hgDestroyDescriptor(desc));

        VkDescriptorImageInfo imageInfo{nullptr, view->view, VK_IMAGE_LAYOUT_GENERAL};
        hgUpdateDescriptor(desc, nullptr, &imageInfo);

        HgPerf perf = hgCreatePerf(arena, iterCount);

        for (u32 i = 0; i < iterCount; ++i)
        {
            u32 seed = std::random_device{}();

            hgBeginPerf(&perf);

            VkCommandBuffer cmd = hgBeginVkCmd();

            HgRenderer renderer = renderer.create(scratch, 4, 4);

            HgRenderPass pass{};
            pass.storageImages = &view;
            pass.storageImageCount = 1;

            renderer.prepareResources(cmd, &pass);

            hgBindComputePipeline(cmd, pipeline, pipelineLayout);

            Push push{};
            push.seed = seed;
            push.imageIdx = hgDescriptorIdx(desc);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(Push), &push);

            vkCmdDispatch(cmd, imageWidth / 16, imageHeight / 16, 1);

            hgEndVkCmd(cmd);

            hgEndPerf(&perf);
        }

        sameImageManual = hgAnalyzePerf(&perf);
    }

    hgDebug("Same Image (Vma)...\n");
    HgPerfStats sameImageVma{};
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgImage* image = hgCreateImage(
            imageWidth,
            imageHeight,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_STORAGE_BIT);
        hgDefer(hgDestroyImage(image));

        HgImageView* view = hgCreateImageView(image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
        hgDefer(hgDestroyImageView(view));

        HgDescriptor desc = hgCreateDescriptor(HgDescriptorType_storageImage);
        hgDefer(hgDestroyDescriptor(desc));

        VkDescriptorImageInfo imageInfo{nullptr, view->view, VK_IMAGE_LAYOUT_GENERAL};
        hgUpdateDescriptor(desc, nullptr, &imageInfo);

        HgPerf perf = hgCreatePerf(arena, iterCount);

        for (u32 i = 0; i < iterCount; ++i)
        {
            u32 seed = std::random_device{}();

            hgBeginPerf(&perf);

            VkCommandBuffer cmd = hgBeginVkCmd();

            HgRenderer renderer = renderer.create(scratch, 4, 4);

            HgRenderPass pass{};
            pass.storageImages = &view;
            pass.storageImageCount = 1;

            renderer.prepareResources(cmd, &pass);

            hgBindComputePipeline(cmd, pipeline, pipelineLayout);

            Push push{};
            push.seed = seed;
            push.imageIdx = hgDescriptorIdx(desc);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(Push), &push);

            vkCmdDispatch(cmd, imageWidth / 16, imageHeight / 16, 1);

            hgEndVkCmd(cmd);

            hgEndPerf(&perf);
        }

        sameImageVma = hgAnalyzePerf(&perf);
    }

    hgDebug("Same 2 Images...\n");
    HgPerfStats same2Images{};
    {
        HgArena* scratch = hgGetScratch();
        HgArenaScope scratchScope{scratch};

        HgImage* images[] = {
            hgCreateImage(
                imageWidth,
                imageHeight,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_STORAGE_BIT),
            hgCreateImage(
                imageWidth,
                imageHeight,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_STORAGE_BIT),
        };
        hgDefer(hgDestroyImage(images[0]));
        hgDefer(hgDestroyImage(images[1]));

        HgImageView* views[] = {
            hgCreateImageView(images[0], {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}),
            hgCreateImageView(images[1], {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}),
        };
        hgDefer(hgDestroyImageView(views[0]));
        hgDefer(hgDestroyImageView(views[1]));

        HgDescriptor descs[] = {
            hgCreateDescriptor(HgDescriptorType_storageImage),
            hgCreateDescriptor(HgDescriptorType_storageImage),
        };
        hgDefer(hgDestroyDescriptor(descs[0]));
        hgDefer(hgDestroyDescriptor(descs[1]));

        VkDescriptorImageInfo imageInfos[] = {
            {nullptr, views[0]->view, VK_IMAGE_LAYOUT_GENERAL},
            {nullptr, views[1]->view, VK_IMAGE_LAYOUT_GENERAL},
        };
        hgUpdateDescriptor(descs[0], nullptr, &imageInfos[0]);
        hgUpdateDescriptor(descs[1], nullptr, &imageInfos[1]);

        HgPerf perf = hgCreatePerf(arena, iterCount);

        u32 frame = 0;

        for (u32 i = 0; i < iterCount; ++i)
        {
            u32 seed = std::random_device{}();

            hgBeginPerf(&perf);

            VkCommandBuffer cmd = hgBeginVkCmd();

            HgRenderer renderer = renderer.create(scratch, 4, 4);

            HgRenderPass pass{};
            pass.storageImages = &views[frame];
            pass.storageImageCount = 1;

            renderer.prepareResources(cmd, &pass);

            hgBindComputePipeline(cmd, pipeline, pipelineLayout);

            Push push{};
            push.seed = seed;
            push.imageIdx = hgDescriptorIdx(descs[frame]);
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(Push), &push);

            vkCmdDispatch(cmd, imageWidth / 16, imageHeight / 16, 1);

            hgEndVkCmd(cmd);

            frame = (frame + 2) % 2;

            hgEndPerf(&perf);
        }

        same2Images = hgAnalyzePerf(&perf);
    }

    HgPerfScale scale = HgPerfScale_milli;
    hgLogPerf("New Allocation (Manual)", &newAllocManual, scale);
    hgLogPerf("   New Allocation (VMA)", &newAllocVma, scale);
    hgLogPerf("     New Image (Manual)", &newImageManual, scale);
    hgLogPerf("    Same Image (Manual)", &sameImageManual, scale);
    hgLogPerf("       Same Image (Vma)", &sameImageVma, scale);
    hgLogPerf("    Same 2 Images (Vma)", &same2Images, scale);
}

