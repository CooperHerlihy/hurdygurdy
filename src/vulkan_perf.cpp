#include "hurdygurdy.hpp"

#include <random>

int main()
{
    hgInit();
    hgDefer(hgDeinit());

    HgArena* arena = hgGetScratch();
    HgArenaScope arenaScope{arena};

    u32 iterCount = 1024;
    u32 imageWidth = 2048;
    u32 imageHeight = 2048;

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

    hgDebug("Same Image...\n");
    HgPerfStats sameImage{};
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

        sameImage = hgAnalyzePerf(&perf);
    }

    // same image
    hgDebug("Same Image (double buffer)...\n");
    HgPerfStats sameImageDoubleBuffer{};
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

        sameImageDoubleBuffer = hgAnalyzePerf(&perf);
    }

    // new image
    hgDebug("New Image...\n");
    HgPerfStats newImage{};
    {
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

    // new allocation, manual
    hgDebug("New Alloc (Manual)...\n");
    HgPerfStats newAllocManual{};
    {
    }

    HgPerfScale scale = HgPerfScale_milli;
    hgLogPerf("                Same Image", &sameImage, scale);
    hgLogPerf("Same Image (Double Buffer)", &sameImageDoubleBuffer, scale);
    hgLogPerf("                 New Image", &newImage, scale);
    hgLogPerf("      New Allocation (VMA)", &newAllocVma, scale);
    hgLogPerf("   New Allocation (Manual)", &newAllocManual, scale);
}
