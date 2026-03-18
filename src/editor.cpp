#include "hurdygurdy.hpp"

#include "stb_image_write.h"

#define IM_ASSERT hgAssert
#include "imgui.h"
#include "imgui_impl_vulkan.h"

int main()
{
    hgDefer(hgDebug("Exited successfully\n"));

    hgInit();
    hgDefer(hgExit());

    hgTest();

    HgArena* arena = hgGetScratch();
    HgArenaScope arenaScope{arena};

    HgWindowConfig windowConfig{};
    windowConfig.title = "Hg Test";
    windowConfig.windowed = true;
    windowConfig.width = 1600;
    windowConfig.height = 900;
    windowConfig.preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    HgWindow* window = HgWindow::create(arena, windowConfig);
    hgDefer(window->destroy());

    HgStringView texturePath = "hg_test_dir/file_image_test.hgtex";
    HgResource textureID = hgResourceID(texturePath);

    HgCreateVkSampler samplerConfig{};
    samplerConfig.filter = VK_FILTER_NEAREST;
    samplerConfig.addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkSampler sampler = hgCreateVkSampler(samplerConfig);
    hgDefer(vkDestroySampler(hgVkDevice, sampler, nullptr));

    {
        HgFence fence;
        hgLoadResource(&fence, 1, textureID, texturePath);
        fence.waitIndefinite();
        hgLoadTexture(textureID, sampler);
        hgUnloadResource(nullptr, 0, textureID);
    }
    hgDefer(hgUnloadTexture(textureID));

    hgInitPipeline2D(arena, 256, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT);
    hgDefer(hgDeinitPipeline2D());

    hgInitPipeline3D(arena, 256, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_D32_SFLOAT);
    hgDefer(hgDeinitPipeline3D());

    hgAddTexture2D(textureID);
    hgDefer(hgRemoveTexture2D(textureID));

    hgAddModel3D(0, 0, 0);
    hgDefer(hgRemoveModel3D(0));

    HgTransform camera{};
    camera.position = HgVec3{0, 0, -2};

    f32 aspectRatio = 16.0f / 9.0f;
    u32 renderWidth = 0;
    u32 renderHeight = 0;

    HgECS ecs = ecs.create(4096);
    hgDefer(ecs.destroy());

    u32 sceneCapacity = 8;
    HgEntity* scene = hgAlloc<HgEntity>(arena, sceneCapacity);
    u32 sceneSize = 0;

    // u32 square = sceneSize++;
    // scene[square] = ecs.spawn();
    // ecs.add<HgTransform>(scene[square]) = {};
    // ecs.get<HgTransform>(scene[square]).position.x = -1.5f;
    // ecs.add<HgSprite>(scene[square]) = {textureID, {0.0f}, {1.0f}};

    // u32 dirLight = sceneSize++;
    // scene[dirLight] = ecs.spawn();
    // ecs.add<HgDirLight3D>(scene[dirLight]) = {{1, 1, 1}, {1, 1, 1, 1}};

    u32 pointLight = sceneSize++;
    scene[pointLight] = ecs.spawn();
    ecs.add<HgTransform>(scene[pointLight]) = {};
    ecs.get<HgTransform>(scene[pointLight]).position = HgVec3{-1, -2, -1};
    ecs.add<HgPointLight3D>(scene[pointLight]) = {HgVec4{1, 1, 1, 2}};

    HgEntity cube = ecs.spawn();
    ecs.add<HgTransform>(cube) = {};
    ecs.add<HgModel3D>(cube) = {};
    u32 cubeIdx = sceneSize++;
    scene[cubeIdx] = cube;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    hgDefer(ImGui::DestroyContext());

    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplHurdyGurdy_Init(window, 1, &window->format);
    hgDefer(ImGui_ImplHurdyGurdy_Shutdown());

    VkSampler renderSampler = nullptr;
    VkImage renderImage = nullptr;
    VmaAllocation renderAlloc = nullptr;
    VkImageView renderView = nullptr;
    VkDescriptorSet renderDescriptor = nullptr;

    VkImage depthImage = nullptr;
    VkImageView depthView = nullptr;
    VmaAllocation depthAlloc = nullptr;

    {
        VkSamplerCreateInfo renderSamplerInfo{};
        renderSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        vkCreateSampler(hgVkDevice, &renderSamplerInfo, nullptr, &renderSampler);
    }
    hgDefer(vkDestroySampler(hgVkDevice, renderSampler, nullptr));
    hgDefer(vmaDestroyImage(hgVkVma, renderImage, renderAlloc));
    hgDefer(vkDestroyImageView(hgVkDevice, renderView, nullptr));
    hgDefer(vmaDestroyImage(hgVkVma, depthImage, depthAlloc));
    hgDefer(vkDestroyImageView(hgVkDevice, depthView, nullptr));

    bool showRender = true;
    bool showEditor = true;
    bool showImguiDemo = false;
    bool move3D = true;
    bool fixedAspect = false;

    HgClock gameClock{};
    HgClock cpuClock{};
    f64 cpuDelta = 0.0;

    for (;;)
    {
        f64 delta = gameClock.tick();

        if (ecs.alive(cube))
        {
            HgQuat& cubeRot = ecs.get<HgTransform>(cube).rotation;
            cubeRot = hgAxisAngle(HgVec3{0, -1, 0}, (f32)delta) * cubeRot;
        }

        HgArena* frame = hgGetScratch(&arena, 1);
        HgArenaScope frameScope{frame};

        hgProcessWindowEvents(&window, 1);
        if (window->wasClosed)
            goto quit;

        ImGui_ImplHurdyGurdy_NewFrame();
        ImGui::NewFrame();

        ImGuiID dockspaceID = ImGui::GetID("dockspace");
        const ImGuiViewport* imguiViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(imguiViewport->WorkPos);
        ImGui::SetNextWindowSize(imguiViewport->WorkSize);
        ImGui::SetNextWindowViewport(imguiViewport->ID);

        ImGuiWindowFlags dockspaceFlags
            = ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Dockspace", nullptr, dockspaceFlags);
        ImGui::PopStyleVar(3);

        ImGui::DockSpace(dockspaceID, {0.0f, 0.0f});

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {

                if (ImGui::MenuItem("Quit"))
                    goto quit;

                if (ImGui::MenuItem("Trigger Trap"))
                    abort();

                if (ImGui::MenuItem("Save Screenshot"))
                {
                    HgArena* scratch = hgGetScratch();
                    HgArenaScope scratchScope{scratch};

                    void* pixels = hgAlloc(scratch, renderWidth * renderHeight * 4, 4);

                    HgReadVkImage config{};
                    config.dst = pixels;
                    config.srcImage = renderImage;
                    config.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    config.subresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
                    config.width = renderWidth;
                    config.height = renderHeight;
                    config.depth = 1;
                    config.format = VK_FORMAT_R8G8B8A8_SRGB;
                    hgReadVkImage(config);

                    stbi_write_png(
                        "screenshot.png",
                        (int)renderWidth,
                        (int)renderHeight,
                        4,
                        pixels,
                        (int)(renderWidth * sizeof(u32)));
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {

                ImGui::Checkbox("Render", &showRender);
                ImGui::Checkbox("Editor", &showEditor);
                ImGui::Checkbox("ImGui Demo", &showImguiDemo);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings"))
            {
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        if (showRender)
        {
            if (ImGui::Begin("Render", &showRender))
            {
                ImVec2 size = ImGui::GetContentRegionAvail();

                u32 viewHeight = fixedAspect ? (u32)std::min(size.y, size.x / aspectRatio) : (u32)size.y;
                u32 viewWidth = fixedAspect ? (u32)((f32)viewHeight * aspectRatio) : (u32)size.x;
                if (renderWidth != viewWidth || renderHeight != viewHeight)
                {
                    vkQueueWaitIdle(hgVkQueue);

                    renderWidth = viewWidth;
                    renderHeight = viewHeight;

                    HgMat4 proj = hgPerspectiveProjection((f32)hgPi * 0.5f, (f32)renderWidth / (f32)renderHeight, 0.1f, 1000.0f);
                    hgUpdateProjection2D(proj);
                    hgUpdateProjection3D(proj);

                    ImGui_ImplVulkan_RemoveTexture(renderDescriptor);
                    vkDestroyImageView(hgVkDevice, renderView, nullptr);
                    vmaDestroyImage(hgVkVma, renderImage, renderAlloc);
                    vkDestroyImageView(hgVkDevice, depthView, nullptr);
                    vmaDestroyImage(hgVkVma, depthImage, depthAlloc);

                    HgCreateVkImage renderImageInfo{};
                    renderImageInfo.width = renderWidth;
                    renderImageInfo.height = renderHeight;
                    renderImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
                    renderImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                            | VK_IMAGE_USAGE_SAMPLED_BIT
                                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

                    hgCreateVkImage(&renderImage, &renderAlloc, renderImageInfo);

                    HgCreateVkImageView renderViewInfo{};
                    renderViewInfo.image = renderImage;
                    renderViewInfo.format = renderImageInfo.format;
                    renderViewInfo.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

                    renderView = hgCreateVkImageView(renderViewInfo);

                    renderDescriptor = ImGui_ImplVulkan_AddTexture(
                        renderSampler, renderView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                    HgCreateVkImage depthImageInfo{};
                    depthImageInfo.width = renderWidth;
                    depthImageInfo.height = renderHeight;
                    depthImageInfo.format = VK_FORMAT_D32_SFLOAT;
                    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

                    hgCreateVkImage(&depthImage, &depthAlloc, depthImageInfo);

                    HgCreateVkImageView depthViewInfo{};
                    depthViewInfo.image = depthImage;
                    depthViewInfo.format = depthImageInfo.format;
                    depthViewInfo.subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                    depthView = hgCreateVkImageView(depthViewInfo);
                }

                if (ImGui::IsWindowFocused())
                {
                    if (move3D && window->isKeyDown[HgKey_lmouse])
                    {
                        f32 rotSpeed = 2.0f;
                        HgQuat rotX = hgAxisAngle(HgVec3{0, 1, 0}, (f32)window->mouseDeltaX * rotSpeed);
                        HgQuat rotY = hgAxisAngle(HgVec3{-1, 0, 0}, (f32)window->mouseDeltaY * rotSpeed);
                        camera.rotation = rotX * camera.rotation * rotY;
                    }

                    HgVec3 movement = HgVec3{0.0f};
                    if (move3D)
                    {
                        movement.y += window->isKeyDown[HgKey_lshift] - window->isKeyDown[HgKey_space];
                        movement.x += window->isKeyDown[HgKey_d] - window->isKeyDown[HgKey_a];
                        movement.z += window->isKeyDown[HgKey_w] - window->isKeyDown[HgKey_s];
                    } else {
                        movement.y += window->isKeyDown[HgKey_s] - window->isKeyDown[HgKey_w];
                        movement.x += window->isKeyDown[HgKey_d] - window->isKeyDown[HgKey_a];
                    }

                    if (movement != HgVec3{0.0f})
                    {
                        f32 moveSpeed = 1.5f;
                        HgVec3 rotated = hgRotate(camera.rotation, HgVec3{movement.x, 0.0f, movement.z});
                        camera.position += hgNorm(HgVec3{rotated.x, movement.y, rotated.z}) * moveSpeed * (f32)delta;
                    }
                }
                HgMat4 view = hgViewMatrix(camera.position, camera.scale, camera.rotation);
                hgUpdateView2D(view);
                hgUpdateView3D(view);

                ImGui::Image((ImTextureID)renderDescriptor, {(f32)renderWidth, (f32)renderHeight});
            }
            ImGui::End();
        }

        if (showEditor)
        {
            if (ImGui::Begin("Editor", &showEditor))
            {
                ImGuiTreeNodeFlags optionsFlags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::CollapsingHeader("Options", optionsFlags))
                {
                    ImGui::SeparatorText("Time");
                    ImGui::Text("total: %.3fms", delta * 1.0e3);
                    ImGui::Text("cpu: %.3fms", cpuDelta * 1.0e3);

                    ImGui::SeparatorText("Camera");
                    if (ImGui::Button("Reset Camera"))
                    {
                        camera.position = HgVec3{0, 0, -1};
                        camera.scale = HgVec3{1, 1, 1};
                        camera.rotation = HgQuat{1, 0, 0, 0};
                    }
                    ImGui::Checkbox("3D Movement", &move3D);
                    ImGui::Checkbox("Fixed Aspect", &fixedAspect);
                    if (fixedAspect)
                        ImGui::DragFloat("Aspect", &aspectRatio, 0.01f, 0.01f, 16.0f);
                }

                ImGuiTreeNodeFlags entityFlags = ImGuiTreeNodeFlags_DefaultOpen;
                if (ImGui::CollapsingHeader("Entities", entityFlags))
                {
                    if (ImGui::Button("Spawn Entity"))
                    {
                        if (sceneSize == sceneCapacity)
                        {
                            scene = hgRealloc(arena, scene, sceneCapacity, sceneCapacity * 2);
                            sceneCapacity *= 2;
                        }
                        scene[sceneSize++] = ecs.spawn();
                    }

                    for (u32 i = 0; i < sceneSize; ++i)
                    {
                        HgArena* scratch = hgGetScratch();
                        HgArenaScope scratchScope{scratch};
                        HgEntity e = scene[i];

                        char* name = HgString::copy(scratch, "Entity ID: ")
                                .append(scratch, hgIntToStr(scratch, (i64)e.idx()))
                                .append(scratch, 0)
                                .chars;

                        if (ImGui::TreeNodeEx(name, entityFlags))
                        {
                            if (ImGui::Button("Despawn Entity"))
                            {
                                ecs.despawn(e);
                                memmove(scene + i, scene + i + 1, sizeof(HgEntity) * (--sceneSize - i));
                                --i;
                            } else {
                                ImGui::SameLine();
                                if (ImGui::Button("Add Component"))
                                    ImGui::OpenPopup("addComponent");
                                if (ImGui::BeginPopup("addComponent"))
                                {
                                    ImGui::SeparatorText("Components");

                                    if (!ecs.has<HgTransform>(e) && ImGui::Selectable("Transform"))
                                        ecs.add<HgTransform>(e) = {};

                                    if (!ecs.has<HgSprite2D>(e) && ImGui::Selectable("Sprite"))
                                    {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgSprite2D>(e) = {textureID, HgVec2{0.0f, 0.0f}, HgVec2{1.0f, 1.0f}};
                                    }

                                    if (!ecs.has<HgSprite2D>(e) && ImGui::Selectable("Model 3D"))
                                    {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgModel3D>(e) = {};
                                    }

                                    if (!ecs.has<HgDirLight3D>(e) && ImGui::Selectable("Direction Light 3D"))
                                        ecs.add<HgDirLight3D>(e) = {};

                                    if (!ecs.has<HgPointLight3D>(e) && ImGui::Selectable("Point Light 3D"))
                                    {
                                        if (!ecs.has<HgTransform>(e))
                                            ecs.add<HgTransform>(e) = {};
                                        ecs.add<HgPointLight3D>(e) = {};
                                    }

                                    ImGui::EndPopup();
                                }

                                ImGui::SameLine();
                                if (ImGui::Button("Remove Component"))
                                    ImGui::OpenPopup("removeComponent");
                                if (ImGui::BeginPopup("removeComponent"))
                                {
                                    ImGui::SeparatorText("Components");

                                    if (ecs.has<HgTransform>(e) && ImGui::Selectable("Transform"))
                                    {
                                        ecs.remove<HgTransform>(e);
                                        if (ecs.has<HgSprite2D>(e))
                                            ecs.remove<HgSprite2D>(e);
                                        if (ecs.has<HgModel3D>(e))
                                            ecs.remove<HgModel3D>(e);
                                        if (ecs.has<HgPointLight3D>(e))
                                            ecs.remove<HgPointLight3D>(e);
                                    }

                                    if (ecs.has<HgSprite2D>(e) && ImGui::Selectable("Sprite"))
                                        ecs.remove<HgSprite2D>(e);

                                    if (ecs.has<HgModel3D>(e) && ImGui::Selectable("Model 3D"))
                                        ecs.remove<HgModel3D>(e);

                                    if (ecs.has<HgDirLight3D>(e) && ImGui::Selectable("Direction Light 3D"))
                                        ecs.remove<HgDirLight3D>(e);

                                    if (ecs.has<HgPointLight3D>(e) && ImGui::Selectable("Point Light 3D"))
                                        ecs.remove<HgPointLight3D>(e);

                                    ImGui::EndPopup();
                                }

                                ImGuiTreeNodeFlags componentFlags = entityFlags;

                                if (ecs.has<HgTransform>(e) && ImGui::TreeNodeEx("Transform", componentFlags))
                                {
                                    HgTransform& tf = ecs.get<HgTransform>(e);
                                    ImGui::DragFloat3("Position", &tf.position.x, 0.01f);
                                    ImGui::DragFloat3("Scale", &tf.scale.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgSprite2D>(e) && ImGui::TreeNodeEx("Sprite", componentFlags))
                                {
                                    HgSprite2D& s = ecs.get<HgSprite2D>(e);
                                    ImGui::DragFloat2("UV Position", &s.uvPos.x, 0.01f);
                                    ImGui::DragFloat2("UV Size", &s.uvSize.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgModel3D>(e) && ImGui::TreeNodeEx("Model 3D", componentFlags))
                                {
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgDirLight3D>(e) && ImGui::TreeNodeEx("Directional Light 3D", componentFlags))
                                {
                                    HgDirLight3D& l = ecs.get<HgDirLight3D>(e);
                                    ImGui::DragFloat3("Direction", &l.dir.x, 0.01f);
                                    ImGui::DragFloat4("Color", &l.color.x, 0.01f);
                                    ImGui::TreePop();
                                }

                                if (ecs.has<HgPointLight3D>(e) && ImGui::TreeNodeEx("Point Light 3D", componentFlags))
                                {
                                    HgPointLight3D& l = ecs.get<HgPointLight3D>(e);
                                    ImGui::DragFloat4("Color", &l.color.x, 0.01f);
                                    ImGui::TreePop();
                                }
                            }
                            ImGui::TreePop();
                        }
                    }
                }
            }
            ImGui::End();
        }

        if (showImguiDemo)
            ImGui::ShowDemoWindow(&showImguiDemo);

        ImGui::Render();

        cpuDelta = cpuClock.tick();
        VkCommandBuffer cmd = window->beginRecording();
        cpuClock.tick();
        if (cmd != nullptr)
        {
            HgRenderer renderer = renderer.create(frame, 64, 64);

            HgImageRenderID renderImageID = renderer.addImage(
                renderImage,
                renderView,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

            HgImageRenderID depthImageID = renderer.addImage(
                depthImage,
                depthView,
                {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});

            HgImageRenderID windowImageID = renderer.addImage(
                window->images[window->currentImage],
                window->views[window->currentImage],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

            {
                HgRenderAttachment renderColorAttachment{};
                renderColorAttachment.image = renderImageID;
                renderColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                renderColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

                HgRenderPass renderPass{};
                renderPass.colorAttachments = &renderColorAttachment;
                renderPass.colorAttachmentCount = 1;
                renderPass.depthAttachment.image = depthImageID;
                renderPass.depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                renderPass.depthAttachment.clearValue.depthStencil = {1.0f, 0};

                renderer.beginPass(cmd, renderWidth, renderHeight, renderPass);

                hgDraw2D(&ecs, cmd);
                hgDraw3D(&ecs, cmd);

                renderer.endPass(cmd);
            }

            {
                HgRenderAttachment guiColorAttachment{};
                guiColorAttachment.image = windowImageID;
                guiColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                guiColorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

                HgRenderPass guiPass{};
                guiPass.sampledImages = &renderImageID;
                guiPass.sampledImageCount = 1;
                guiPass.colorAttachments = &guiColorAttachment;
                guiPass.colorAttachmentCount = 1;

                renderer.beginPass(cmd, window->width, window->height, guiPass);

                ImGui_ImplHurdyGurdy_Draw(cmd);

                renderer.endPass(cmd);
            }

            HgImageBarrier presentBarrier{};
            presentBarrier.image = windowImageID;
            presentBarrier.nextUsage = HgRenderUsage_presentSrc;

            renderer.barrier(cmd, nullptr, 0, &presentBarrier, 1);

            window->endAndPresent(cmd);
        }
    }
quit:
    vkDeviceWaitIdle(hgVkDevice);
}

