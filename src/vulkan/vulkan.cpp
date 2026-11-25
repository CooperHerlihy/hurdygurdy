#include "vulkan_gpu.hpp"

#include "hg/defer.hpp"
#include "hg/logging.hpp"
#include "hg/memory.hpp"
#include "hg/window.hpp"

#include <algorithm>
#include <cstring>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

namespace hg {

namespace internal {

void* getVkInstance(Gpu* gpu)
{
    return static_cast<vk::GpuVk*>(gpu)->instance;
}

void* getVkInstanceProcAddr(Gpu* gpu, const char* name)
{
    return reinterpret_cast<void*>(vkGetInstanceProcAddr(static_cast<vk::GpuVk*>(gpu)->instance, name));
}

} // namespace internal

namespace vk {

bool loadVulkan();
void unloadVulkan();
bool loadVulkanInstanceFuncs(GpuVk* instance);
bool loadVulkanDeviceFuncs(GpuVk* device);

static const char* deviceExtensions[]{
    "VK_KHR_swapchain",
};

static VkBool32 debugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    const VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    static_cast<void>(type);
    static_cast<void>(userData);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Error: %s\n", callbackData->pMessage));
        abort();
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Warning: %s\n", callbackData->pMessage));
        if (callbackData->pMessage == std::string("DRM_NOUVEAU_EXEC failed: No such device (VK_ERROR_DEVICE_LOST)"))
            abort();
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Info: %s\n", callbackData->pMessage));
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        static_cast<void>(fprintf(stderr, "Vulkan Verbose: %s\n", callbackData->pMessage));
    } else {
        static_cast<void>(fprintf(stderr, "Vulkan Unknown: %s\n", callbackData->pMessage));
    }
    return VK_FALSE;
}

static const VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr,
    0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    debugCallback,
    nullptr,
};

static bool isDebugMessengerAvailable()
{
#ifdef HG_VK_DEBUG_MESSENGER
    ArenaScope scratch = getScratch();

    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    VectorTemp<VkLayerProperties> layers{layerCount, scratch};
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    if (std::ranges::none_of(layers, [&](VkLayerProperties& layer)
        {
            return strcmp(layer.layerName,  "VK_LAYER_KHRONOS_validation") == 0;
        }))
        return false;

    u32 extCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    VectorTemp<VkExtensionProperties> exts{extCount, scratch};
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());

    if (std::ranges::none_of(exts, [&](VkExtensionProperties& ext)
        {
            return strcmp(ext.extensionName, "VK_EXT_debug_utils") == 0;
        }))
        return false;

    return true;
#endif
    return false;
}

static VkInstance createInstance(GpuVk* vk, std::span<std::string_view> extensions)
{
    ArenaScope scratch = getScratch();

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hurdy Gurdy Application",
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = "Hurdy Gurdy Engine";
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;

    if (vk->enableDebugMessenger)
    {
        instanceInfo.pNext = &debugUtilsMessengerInfo;
        // Must outlive the vkCreateInstance call: a stack array can have its
        // slot reused by the compiler before the loader reads the pointer.
        static constexpr const char* layers[]{
            "VK_LAYER_KHRONOS_validation",
        };
        instanceInfo.enabledLayerCount = assertCast<u32>(std::size(layers));
        instanceInfo.ppEnabledLayerNames = layers;
    }

    VectorTemp<StringTemp> extStrs{scratch};
    extStrs.reserve(extensions.size());
    VectorTemp<const char*> extCStrs{scratch};
    extCStrs.reserve(extensions.size() + 1);
    for (u32 i = 0; i < extensions.size(); ++i)
    {
        extStrs.emplace_back(extensions[i], scratch);
        extCStrs.push_back(extStrs.back().c_str());
    }
    if (vk->enableDebugMessenger)
        extCStrs.emplace_back("VK_EXT_debug_utils");
    instanceInfo.enabledExtensionCount = assertCast<u32>(extCStrs.size());
    instanceInfo.ppEnabledExtensionNames = extCStrs.data();

    VkInstance instance = nullptr;
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (instance == nullptr)
    {
        setError("Failed to create Vulkan instance: {}", vkResultToStr(result));
    }

    return instance;
}

static VkDebugUtilsMessengerEXT createDebugUtilsMessenger(GpuVk* vk)
{
    HG_ASSERT(vk->instance != nullptr);

    VkDebugUtilsMessengerEXT messenger = nullptr;
    VkResult result = vkCreateDebugUtilsMessengerEXT(vk->instance, &debugUtilsMessengerInfo, nullptr, &messenger);
    if (messenger == nullptr)
    {
        setError("Failed to create Vulkan debug messenger: {}", vkResultToStr(result));
    }

    return messenger;
}

static bool findQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags)
{
    HG_ASSERT(gpu != nullptr);
    HG_ASSERT(queueFamily != nullptr);

    ArenaScope scratch = getScratch();

    u32 familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    VectorTemp<VkQueueFamilyProperties> families{familyCount, scratch};
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());

    auto it = std::ranges::find_if(families, [&](VkQueueFamilyProperties& family)
    {
        return (family.queueFlags & queueFlags) != 0;
    });
    if (it == families.end())
    {
        setError("Could not find Vulkan queue family");
        *queueFamily = (u32)-1;
        return false;
    }

    *queueFamily = assertCast<u32>(it - families.begin());
    return true;
}

static VkPhysicalDevice findPhysicalDevice(GpuVk* vk)
{
    HG_ASSERT(vk->instance != nullptr);

    ArenaScope scratch = getScratch();

    u32 gpuCount;
    vkEnumeratePhysicalDevices(vk->instance, &gpuCount, nullptr);
    VectorTemp<VkPhysicalDevice> gpus{gpuCount, scratch};
    vkEnumeratePhysicalDevices(vk->instance, &gpuCount, gpus.data());

    VectorTemp<VkExtensionProperties> extProps{scratch};
    auto it = std::ranges::find_if(gpus, [&](VkPhysicalDevice gpu)
    {
        u32 propCount = 0;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propCount, nullptr);
        extProps.resize(propCount);
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propCount, extProps.data());

        if (std::ranges::any_of(deviceExtensions, [&](const char* extName)
            {
                return std::ranges::none_of(extProps, [&](VkExtensionProperties& extProp)
                {
                    return strcmp(extName, extProp.extensionName) == 0;
                });
            }))
            return false;

        u32 family;
        if (!findQueueFamily(gpu, &family, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT))
            return false;

        return true;
    });
    if (it == gpus.end())
    {
        setError("Could not find suitable gpu");
        return nullptr;
    }
    return *it;
}

static VkDevice createDevice(GpuVk* vk)
{
    HG_ASSERT(vk->physicalDevice != nullptr);
    HG_ASSERT(vk->queueFamily != (u32)-1);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeature{};
    descriptorIndexingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexingFeature.pNext = nullptr;
    descriptorIndexingFeature.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexingFeature.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptorIndexingFeature.runtimeDescriptorArray = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.pNext = &descriptorIndexingFeature;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features synchronization2Feature{};
    synchronization2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Feature.pNext = &dynamicRenderingFeature;
    synchronization2Feature.synchronization2 = VK_TRUE;

    VkPhysicalDeviceFeatures features{};

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = vk->queueFamily;
    queueInfo.queueCount = 1;
    f32 queuePriority = 1.0f;
    queueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &synchronization2Feature;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.pEnabledFeatures = &features;
    if (vk->enableSurfaces)
    {
        deviceInfo.enabledExtensionCount = assertCast<u32>(std::size(deviceExtensions));
        deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    }

    VkDevice device = nullptr;
    VkResult result = vkCreateDevice(vk->physicalDevice, &deviceInfo, nullptr, &device);
    if (device == nullptr)
    {
        setError("Could not create VkDevice: {}", vkResultToStr(result));
    }

    return device;
}

static VmaAllocator createVma(GpuVk* vk)
{
    HG_ASSERT(vk->instance != nullptr);
    HG_ASSERT(vk->physicalDevice != nullptr);
    HG_ASSERT(vk->device != nullptr);

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = vk->physicalDevice;
    allocatorInfo.device = vk->device;
    allocatorInfo.instance = vk->instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator vma = nullptr;
    VkResult result = vmaCreateAllocator(&allocatorInfo, &vma);
    if (vma == nullptr)
    {
        setError("Could not create Vulkan memory allocator: {}", vkResultToStr(result));
    }

    return vma;
}

static VkDescriptorPool createBindlessDescriptorPool(GpuVk* vk)
{
    VkDescriptorPoolSize sizes[]{
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, UINT16_MAX},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, UINT16_MAX},
    };

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    info.maxSets = 1;
    info.poolSizeCount = assertCast<u32>(std::size(sizes));
    info.pPoolSizes = sizes;

    VkDescriptorPool pool = nullptr;
    [[maybe_unused]] VkResult result = vkCreateDescriptorPool(vk->device, &info, nullptr, &pool);
    if (pool == nullptr)
        panic("Could not create VkDescriptorPool: {}\n", vkResultToStr(result));

    return pool;
}

static VkDescriptorSetLayout createBindlessDescriptorLayout(GpuVk* vk)
{
    VkDescriptorSetLayoutBinding bindings[static_cast<u32>(DescriptorType::count)]{};
    VkDescriptorBindingFlags flags[static_cast<u32>(DescriptorType::count)]{};
    for (u32 i = 0; i < static_cast<u32>(DescriptorType::count); ++i)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = descriptorTypeToVk(static_cast<DescriptorType>(i));
        bindings[i].descriptorCount = UINT16_MAX;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
                 | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    flagsInfo.bindingCount = assertCast<u32>(std::size(bindings));
    flagsInfo.pBindingFlags = flags;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = &flagsInfo;
    info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    info.bindingCount = assertCast<u32>(std::size(bindings));
    info.pBindings = bindings;

    VkDescriptorSetLayout layout = nullptr;
    [[maybe_unused]] VkResult result = vkCreateDescriptorSetLayout(vk->device, &info, nullptr, &layout);
    if (layout == nullptr)
        panic("Could not create bindless VkDescriptorSetLayout: {}\n", vkResultToStr(result));

    return layout;
}

static Frame createFrame(GpuVk* vk)
{
    Frame frame{};

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vk->queueFamily;

    [[maybe_unused]]
    VkResult poolResult = vkCreateCommandPool(vk->device, &poolInfo, nullptr, &frame.cmdPool);
    if (frame.cmdPool == nullptr)
        panic("Could not create Vulkan command pool: {}\n", vkResultToStr(poolResult));

    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = frame.cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    [[maybe_unused]]
    VkResult cmdResult = vkAllocateCommandBuffers(vk->device, &cmdInfo, &frame.cmd);
    if (frame.cmd == nullptr)
        panic("Could not create Vulkan command buffer: {}\n", vkResultToStr(cmdResult));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    [[maybe_unused]]
    VkResult fenceResult = vkCreateFence(vk->device, &fenceInfo, nullptr, &frame.fence);
    if (frame.fence == nullptr)
        panic("Could not create Vulkan fence: {}\n", vkResultToStr(fenceResult));

    return frame;
}

u32 DescriptorPool::alloc()
{
    if (freed.empty())
    {
        return next++;
    }
    else
    {
        u32 desc = freed.back();
        freed.pop_back();
        return desc;
    }
}

void DescriptorPool::free(u32 desc)
{
    HG_ASSERT(desc < next);
    HG_ASSERT(!std::ranges::any_of(freed, [&](u32 f) { return desc == f; }));
    freed.emplace_back(desc);
}

static Descriptor createBufferDescriptor(
    GpuVk* vk,
    DescriptorType type,
    const GpuBufferVk* buffer,
    u64 offset,
    u64 range)
{
    HG_ASSERT(type < DescriptorType::count);

    ArenaScope scratch = getScratch();

    Descriptor desc = vk->descriptorPools[static_cast<u32>(type)].alloc();

    VkDescriptorBufferInfo bufferInfo{buffer->buffer, offset, range};

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk->bindlessSet;
    write.dstBinding = static_cast<u32>(type);
    write.dstArrayElement = desc;
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = &bufferInfo;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk->device, 1, &write, 0, nullptr);

    return desc;
}

static Descriptor createImageDescriptor(
    GpuVk* vk,
    DescriptorType type,
    const GpuViewVk* imageView,
    GpuLayout imageLayout)
{
    HG_ASSERT(type < DescriptorType::count);

    ArenaScope scratch = getScratch();

    Descriptor desc = vk->descriptorPools[static_cast<u32>(type)].alloc();

    VkDescriptorImageInfo imageInfo{
        imageView->sampler,
        imageView->view,
        gpuLayoutToVk(imageLayout)
    };

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk->bindlessSet;
    write.dstBinding = static_cast<u32>(type);
    write.dstArrayElement = desc;
    write.descriptorCount = 1;
    write.descriptorType = descriptorTypeToVk(type);
    write.pBufferInfo = nullptr;
    write.pImageInfo = &imageInfo;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(vk->device, 1, &write, 0, nullptr);

    return desc;
}

static void descriptorDestroy(GpuVk* vk, Descriptor desc, DescriptorType type)
{
    vk->descriptorPools[static_cast<u32>(type)].free(desc);
}

static VkSampler samplerCreate(GpuVk* vk, SamplerInfo* desc)
{
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = gpuFilterToVk(desc->filter);
    info.minFilter = gpuFilterToVk(desc->filter);
    info.mipmapMode = desc->filter == GpuFilter::linear
        ? VK_SAMPLER_MIPMAP_MODE_LINEAR
        : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU = gpuSamplerAddressModeToVk(desc->mode);
    info.addressModeV = gpuSamplerAddressModeToVk(desc->mode);
    info.addressModeW = gpuSamplerAddressModeToVk(desc->mode);
    info.mipLodBias = 0.0f;
    info.minLod = 0.0f;
    info.maxLod = 1000.0f;
    info.borderColor = gpuSamplerBorderToVk(desc->border);

    VkSampler sampler = nullptr;
    [[maybe_unused]]     VkResult result = vkCreateSampler(vk->device, &info, nullptr, &sampler);
    if (sampler == nullptr)
        panic("Could not create VkSampler: {}\n", vkResultToStr(result));

    return sampler;
}

static VkSampler samplerGet(
    GpuVk* vk,
    GpuFilter filter,
    GpuSamplerEdgeMode addressMode = GpuSamplerEdgeMode::repeat,
    GpuSamplerBorder borderColor = GpuSamplerBorder::floatTransparentBlack)
{
    SamplerInfo desc = {filter, addressMode, borderColor};
    VkSampler* sampler = vk->samplers.get(desc);
    if (sampler == nullptr)
    {
        sampler = vk->samplers.add(desc, samplerCreate(vk, &desc));
    }
    return *sampler;
}

std::unique_ptr<GpuVk> GpuVk::create(std::span<std::string_view> extensions)
{
    if (!loadVulkan())
        return nullptr;

    std::unique_ptr vk = std::make_unique<GpuVk>();

    vk->enableDebugMessenger = isDebugMessengerAvailable();
    vk->enableSurfaces = false;

    for (std::string_view extension : extensions)
    {
        if (extension == "VK_KHR_surface")
        {
            vk->enableSurfaces = true;
            break;
        }
    }

    vk->instance = createInstance(vk.get(), extensions);
    if (vk->instance == nullptr)
        return nullptr;
    if (!loadVulkanInstanceFuncs(vk.get()))
        return nullptr;

    if (vk->enableDebugMessenger)
    {
        vk->debugMessenger = createDebugUtilsMessenger(vk.get());
        if (vk->debugMessenger == nullptr)
            return nullptr;
    }

    vk->physicalDevice = findPhysicalDevice(vk.get());
    if (vk->physicalDevice == nullptr)
        return nullptr;

    findQueueFamily(vk->physicalDevice, &vk->queueFamily,
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);
    if (vk->queueFamily == (u32)-1)
        return nullptr;

    vk->device = createDevice(vk.get());
    if (vk->device == nullptr)
        return nullptr;

    if (!loadVulkanDeviceFuncs(vk.get()))
        return nullptr;

    vkGetDeviceQueue(vk->device, vk->queueFamily, 0, &vk->queue);
    if (vk->queue == nullptr)
        return nullptr;

    vk->vma = createVma(vk.get());
    if (vk->vma == nullptr)
        return nullptr;

    {
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = vk->queueFamily;

        [[maybe_unused]]         VkResult result = vkCreateCommandPool(vk->device, &cmdPoolInfo, nullptr, &vk->cmdPool);
        if (vk->cmdPool == nullptr)
            panic("Could not create Vulkan command pool: {}\n", vkResultToStr(result));
    }

    vk->bindlessPool = createBindlessDescriptorPool(vk.get());
    vk->bindlessLayout = createBindlessDescriptorLayout(vk.get());
    {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = vk->bindlessPool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &vk->bindlessLayout;

        [[maybe_unused]]         VkResult result = vkAllocateDescriptorSets(vk->device, &info, &vk->bindlessSet);
        if (vk->bindlessSet == nullptr)
            panic("Could not allocate bindless VkDescriptorSet: {}\n", vkResultToStr(result));
    }

    for (u32 i = 0; i < static_cast<u32>(DescriptorType::count); ++i)
    {
        vk->descriptorPools[i] = {};
    }

    vk->samplers = HashMap<SamplerInfo, VkSampler>(
        2 *
        static_cast<u32>(GpuFilter::count) *
        static_cast<u32>(GpuSamplerEdgeMode::count) *
        static_cast<u32>(GpuSamplerBorder::count));

    vk->frames.resize(2);
    for (Frame& frame : vk->frames)
    {
        frame = createFrame(vk.get());
    }
    vk->currentFrame = 0;

    return vk;
}

GpuVk::~GpuVk() noexcept
{
    vkDeviceWaitIdle(device);

    for (u32 i = 0; i < frames.size(); ++i)
    {
        vkDestroyFence(device, frames[i].fence, nullptr);
        vkDestroyCommandPool(device, frames[i].cmdPool, nullptr);
        frames[i].swapchains = {};
    }

    samplers.forEach([&](const SamplerInfo&, VkSampler& sampler)
    {
        vkDestroySampler(device, sampler, nullptr);
    });
    samplers = {};

    for (u32 i = 0; i < static_cast<u32>(DescriptorType::count); ++i)
    {
        descriptorPools[i] = {};
    }

    vkDestroyDescriptorSetLayout(device, bindlessLayout, nullptr);
    vkDestroyDescriptorPool(device, bindlessPool, nullptr);

    vkDestroyCommandPool(device, cmdPool, nullptr);

    vmaDestroyAllocator(vma);

    vkDestroyDevice(device, nullptr);

    if (enableDebugMessenger)
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

    vkDestroyInstance(instance, nullptr);

    unloadVulkan();
}

static VkShaderModule createShaderModule(GpuVk* vk, std::span<const byte> spirvCode)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = spirvCode.size();
    info.pCode = (const u32*)spirvCode.data();

    VkShaderModule shader = nullptr;
    [[maybe_unused]] VkResult result = vkCreateShaderModule(vk->device, &info, nullptr, &shader);
    if (shader == nullptr)
        panic("Could not create VkShaderModule: {}\n", vkResultToStr(result));

    return shader;
}

static Format findSwapchainFormat(GpuVk* vk, VkSurfaceKHR surface)
{
    HG_ASSERT(surface != nullptr);

    ArenaScope scratch = getScratch();

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk->physicalDevice, surface, &formatCount, nullptr);
    VectorTemp<VkSurfaceFormatKHR> formats{scratch};
    formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk->physicalDevice, surface, &formatCount, formats.data());

    for (VkSurfaceFormatKHR& f : formats)
    {
        if (f.format == VK_FORMAT_R8G8B8A8_SRGB)
            return Format::rgba8_srgb;
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB)
            return Format::bgra8_srgb;
    }
    panic("No supported swapchain formats\n");
}

static GpuPresentMode findSwapchainPresentMode(
    GpuVk* vk,
    VkSurfaceKHR surface,
    GpuPresentMode desiredMode)
{
    HG_ASSERT(surface != nullptr);

    ArenaScope scratch = getScratch();

    if (desiredMode == GpuPresentMode::fifo)
        return desiredMode;

    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk->physicalDevice, surface, &modeCount, nullptr);
    VectorTemp<VkPresentModeKHR> presentModes{scratch};
    presentModes.resize(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk->physicalDevice, surface, &modeCount, presentModes.data());

    for (u32 i = 0; i < modeCount; ++i)
    {
        if (presentModes[i] == presentModeToVk(desiredMode))
            return desiredMode;
    }
    return GpuPresentMode::fifo;
}

void GpuVk::waitIdle()
{
    vkQueueWaitIdle(queue);
}

std::unique_ptr<GpuBuffer> GpuVk::createBuffer(
    u64 size,
    GpuBufferUsageFlags usageFlags,
    GpuMemoryUsage memoryUsage)
{
    HG_ASSERT(size > 0);
    HG_ASSERT(usageFlags != 0);

    std::unique_ptr<GpuBufferVk> buf = std::make_unique<GpuBufferVk>();
    buf->vk = this;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = gpuBufferUsageToVk(usageFlags);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = gpuMemoryUsageToVma(memoryUsage);

    VkResult result = vmaCreateBuffer(
        vma,
        &bufferInfo,
        &allocInfo,
        &buf->buffer,
        &buf->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        panic("Could not create VkBuffer: {}\n", vkResultToStr(result));

    buf->bufSize = size;

    if (usageFlags & GpuBufferUsage::uniformBuffer)
        buf->uniformDesc = createBufferDescriptor(this, DescriptorType::uniformBuffer, buf.get(), 0, size);

    if (usageFlags & GpuBufferUsage::storageBuffer)
        buf->storageDesc = createBufferDescriptor(this, DescriptorType::storageBuffer, buf.get(), 0, size);

    buf->usage = usageFlags;

    if (memoryUsage == GpuMemoryUsage::frequentUpdate)
    {
        VkMemoryPropertyFlags memPropFlags;
        vmaGetAllocationMemoryProperties(vma, buf->alloc, &memPropFlags);
        buf->memoryAccess = memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            ? GpuMemoryHostAccess::write
            : GpuMemoryHostAccess::none;
    } else {
        buf->memoryAccess = gpuMemoryUsageToHostAccess(memoryUsage);
    }

    return buf;
}

std::unique_ptr<GpuImage> GpuVk::createImage(
    u32 width,
    u32 height,
    Format format,
    GpuImageUsageFlags usage)
{
    GpuImageCreateInfo create{};
    create.width = width;
    create.height = height;
    create.format = format;
    create.usage = usage;
    return createImage(create);
}

std::unique_ptr<GpuImage> GpuVk::createImage(const GpuImageCreateInfo& create)
{
    HG_ASSERT(create.format != Format::undefined);
    HG_ASSERT(create.usage != 0);

    std::unique_ptr<GpuImageVk> img = std::make_unique<GpuImageVk>();
    img->vk = this;

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
        vma,
        &imageInfo,
        &allocInfo,
        &img->image,
        &img->alloc,
        nullptr);

    if (result != VK_SUCCESS)
        panic("Could not create VkImage: {}\n", vkResultToStr(result));

    img->usage = create.usage;
    img->imageFormat = create.format;
    img->imageWidth = create.width;
    img->imageHeight = create.height;
    img->imageDepth = create.dimensions >= 3 ? create.depth : 1;
    img->dimensions = assertCast<u8>(create.dimensions);
    img->mipLevels = assertCast<u8>(create.mipLevels);
    img->arrayLayers = assertCast<u8>(create.arrayLayers);
    img->msaaSamples = assertCast<u8>(create.msaaSamples);

    return img;
}

std::unique_ptr<GpuPipeline> GpuVk::createGraphicsPipeline(const GpuGraphicsPipelineCreateInfo& config)
{
    HG_ASSERT(config.vertexShader.size() > 0);
    HG_ASSERT(config.fragmentShader.size() > 0);

    std::unique_ptr<GpuPipelineVk> pipe = std::make_unique<GpuPipelineVk>();
    pipe->vk = this;
    pipe->pushSize = config.pushConstantSize;

    ArenaScope scratch = getScratch();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &bindlessLayout;
    VkPushConstantRange pushRange{VK_SHADER_STAGE_ALL, 0, config.pushConstantSize};
    if (config.pushConstantSize > 0)
    {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
    }

    [[maybe_unused]]
    VkResult layoutResult = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipe->layout);
    if (pipe->layout == nullptr)
        panic("Could not create VkPipelineLayout: {}\n", vkResultToStr(layoutResult));

    VkShaderModule vertexShader = createShaderModule(this, config.vertexShader);
    VkShaderModule fragmentShader = createShaderModule(this, config.fragmentShader);
    HG_DEFER(vkDestroyShaderModule(device, vertexShader, nullptr));
    HG_DEFER(vkDestroyShaderModule(device, fragmentShader, nullptr));

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

    VectorTemp<VkPipelineColorBlendAttachmentState> colorBlendAttachments{scratch};
    colorBlendAttachments.reserve(config.colorAttachmentFormats.size());

    for (u32 i = 0; i < config.colorAttachmentFormats.size(); ++i)
    {
        VkPipelineColorBlendAttachmentState& attachment = colorBlendAttachments.emplace_back();
        attachment.blendEnable = config.colorBlendEnables.data() != nullptr
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
    colorBlendState.attachmentCount = assertCast<u32>(colorBlendAttachments.size());
    colorBlendState.pAttachments = colorBlendAttachments.data();
    for (float& blendConstant : colorBlendState.blendConstants)
    {
        blendConstant = 1.0f;
    }

    VkDynamicState dynamicStates[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = assertCast<u32>(std::size(dynamicStates));
    dynamicState.pDynamicStates = dynamicStates;

    VectorTemp<VkFormat> colorFormats{scratch};
    colorFormats.reserve(assertCast<u32>(config.colorAttachmentFormats.size()));
    for (u32 i = 0; i < config.colorAttachmentFormats.size(); ++i)
    {
        colorFormats.push_back(formatToVk(config.colorAttachmentFormats[i]));
    }
    VkFormat depthFormat = formatToVk(config.depthAttachmentFormat);
    VkFormat stencilFormat = formatToVk(config.stencilAttachmentFormat);

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = assertCast<u32>(colorFormats.size());
    renderingInfo.pColorAttachmentFormats = colorFormats.data();
    renderingInfo.depthAttachmentFormat = depthFormat;
    renderingInfo.stencilAttachmentFormat = stencilFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = assertCast<u32>(std::size(shaderStages));
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
    pipelineInfo.layout = pipe->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    [[maybe_unused]]
    VkResult pipelineResult = vkCreateGraphicsPipelines(
        device, nullptr, 1, &pipelineInfo, nullptr, &pipe->pipeline);
    if (pipe->pipeline == nullptr)
        panic("Failed to create Vulkan graphics pipeline: {}\n", vkResultToStr(pipelineResult));

    pipe->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    return pipe;
}

std::unique_ptr<GpuPipeline> GpuVk::createComputePipeline(std::span<const byte> shaderCode, u32 pushSize)
{
    HG_ASSERT(shaderCode.data() != nullptr);
    HG_ASSERT(shaderCode.size() > 0);

    std::unique_ptr<GpuPipelineVk> pipe = std::make_unique<GpuPipelineVk>();
    pipe->vk = this;
    pipe->pushSize = pushSize;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &bindlessLayout;

    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, pushSize};
    layoutInfo.pushConstantRangeCount = pushSize > 0 ? 1 : 0;
    layoutInfo.pPushConstantRanges = pushSize > 0 ? &push : nullptr;

    [[maybe_unused]]
    VkResult layoutResult = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipe->layout);
    if (pipe->layout == nullptr)
        panic("Could not create VkPipelineLayout: {}\n", vkResultToStr(layoutResult));

    VkShaderModule computeShader = createShaderModule(this, shaderCode);
    HG_DEFER(vkDestroyShaderModule(device, computeShader, nullptr));

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = computeShader;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = pipe->layout;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    [[maybe_unused]]
    VkResult pipelineResult = vkCreateComputePipelines(
        device, nullptr, 1, &pipelineInfo, nullptr, &pipe->pipeline);
    if (pipe->pipeline == nullptr)
        panic("Failed to create Vulkan compute pipeline: {}\n", vkResultToStr(pipelineResult));

    pipe->bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

    return pipe;
}

std::unique_ptr<GpuCommandBuffer> GpuVk::beginCommands()
{
    VkCommandBufferAllocateInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.commandPool = cmdPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = nullptr;
    vkAllocateCommandBuffers(device, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);

    std::unique_ptr<GpuCommandBufferVk> result = std::make_unique<GpuCommandBufferVk>();
    result->vk = this;
    result->cmd = cmd;
    return result;
}

void GpuVk::endCommands(std::unique_ptr<GpuCommandBuffer> cmd)
{
    HG_ASSERT(cmd != nullptr);

    GpuCommandBufferVk* commandBuffer = static_cast<GpuCommandBufferVk*>(cmd.get());
    HG_ASSERT(commandBuffer->cmd != nullptr);

    vkEndCommandBuffer(commandBuffer->cmd);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);
    HG_DEFER(vkDestroyFence(device, fence, nullptr));

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer->cmd;

    vkQueueSubmit(queue, 1, &submit, fence);

    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);

    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer->cmd);
    commandBuffer->cmd = nullptr;
}

std::unique_ptr<GpuCommandBuffer> GpuVk::beginFrame(std::span<Window*> windows)
{
    Frame* frame = &frames[currentFrame];

    vkWaitForFences(device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frame->fence);

    frame->swapchains.clear();
    for (u32 i = 0; i < windows.size(); ++i)
    {
        GpuSwapchainVk* swap = static_cast<GpuSwapchainVk*>(windows[i]->swapchain());
        if (swap->swapchain == nullptr)
            continue;

        VkResult result = vkAcquireNextImageKHR(
            device,
            swap->swapchain,
            UINT64_MAX,
            swap->imageAvailable[currentFrame],
            nullptr,
            &swap->imageIdx);

        if (result == VK_SUCCESS)
        {
            frame->swapchains.push_back(swap);
        }
        else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            swap->resize(swap->imageWidth, swap->imageHeight);
            swap->imageIdx = (u32)-1;
        }
        else
        {
            panic("Could not acquire next image: {}\n", vkResultToStr(result));
        }
    }

    vkResetCommandPool(device, frame->cmdPool, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(frame->cmd, &beginInfo);

    std::unique_ptr<GpuCommandBufferVk> result = std::make_unique<GpuCommandBufferVk>();
    result->vk = this;
    result->cmd = frame->cmd;
    return result;
}

void GpuVk::endFrame(std::unique_ptr<GpuCommandBuffer> cmd)
{
    HG_ASSERT(cmd != nullptr);

    ArenaScope scratch = getScratch();

    Frame* frame = &frames[currentFrame];
    GpuCommandBufferVk* commandBuffer = static_cast<GpuCommandBufferVk*>(cmd.get());

    VectorTemp<GpuImageBarrier> presentBarriers{scratch};
    presentBarriers.reserve(frame->swapchains.size());
    for (GpuSwapchain* swapBase : frame->swapchains)
    {
        GpuSwapchainVk* swap = static_cast<GpuSwapchainVk*>(swapBase);
        GpuImageBarrier& barrier = presentBarriers.emplace_back();
        barrier.image = &swap->views[swap->imageIdx];
        barrier.nextLayout = GpuLayout::presentSrc;
    }
    cmd->memoryBarrier(
        std::span<const GpuBufferBarrier>{},
        std::span<const GpuImageBarrier>{presentBarriers.data(), presentBarriers.size()});

    vkEndCommandBuffer(commandBuffer->cmd);

    VectorTemp<VkPipelineStageFlags> waitStages{scratch};
    waitStages.reserve(frame->swapchains.size());
    VectorTemp<VkSemaphore> imageAvailableSemaphores{scratch};
    imageAvailableSemaphores.reserve(frame->swapchains.size());
    VectorTemp<VkSemaphore> readyToPresentSemaphores{scratch};
    readyToPresentSemaphores.reserve(frame->swapchains.size());

    VectorTemp<VkSwapchainKHR> swapchains{scratch};
    swapchains.reserve(frame->swapchains.size());
    VectorTemp<u32> imageIndices{scratch};
    imageIndices.reserve(frame->swapchains.size());

    for (GpuSwapchain* swapBase : frame->swapchains)
    {
        GpuSwapchainVk* swap = static_cast<GpuSwapchainVk*>(swapBase);
        waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        imageAvailableSemaphores.push_back(swap->imageAvailable[currentFrame]);
        readyToPresentSemaphores.push_back(swap->readyToPresent[swap->imageIdx]);
        swapchains.push_back(swap->swapchain);
        imageIndices.push_back(swap->imageIdx);
        swap->imageIdx = (u32)-1;
    }

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = assertCast<u32>(imageAvailableSemaphores.size());
    submit.pWaitSemaphores = imageAvailableSemaphores.data();
    submit.pWaitDstStageMask = waitStages.data();
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer->cmd;
    submit.signalSemaphoreCount = assertCast<u32>(readyToPresentSemaphores.size());
    submit.pSignalSemaphores = readyToPresentSemaphores.data();

    vkQueueSubmit(queue, 1, &submit, frame->fence);
    commandBuffer->cmd = nullptr;

    if (frame->swapchains.size() > 0)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = assertCast<u32>(readyToPresentSemaphores.size());
        presentInfo.pWaitSemaphores = readyToPresentSemaphores.data();
        presentInfo.swapchainCount = assertCast<u32>(swapchains.size());
        presentInfo.pSwapchains = swapchains.data();
        presentInfo.pImageIndices = imageIndices.data();

        vkQueuePresentKHR(queue, &presentInfo);
    }

    currentFrame = (currentFrame + 1) % frames.size();
}

std::unique_ptr<GpuSwapchain> GpuVk::createSwapchain(
    void* surface,
    u32 width,
    u32 height,
    GpuPresentMode presentMode,
    GpuImageUsageFlags imageUsage)
{
    std::unique_ptr<GpuSwapchainVk> swap = std::make_unique<GpuSwapchainVk>();
    swap->vk = this;
    swap->surface = static_cast<VkSurfaceKHR>(surface);
    swap->imageFormat = findSwapchainFormat(this, swap->surface);
    swap->presentMode = findSwapchainPresentMode(this, swap->surface, presentMode);
    swap->imageUsage = imageUsage;
    swap->imageAvailable = std::vector<VkSemaphore>(frames.size(), nullptr);

    swap->resize(width, height);
    return swap;
}

u64 GpuBufferVk::size() const
{
    return bufSize;
}

u32 GpuBufferVk::uniformDescriptor() const
{
    HG_ASSERT(usage & GpuBufferUsage::uniformBuffer);
    return uniformDesc;
}

u32 GpuBufferVk::storageDescriptor() const
{
    HG_ASSERT(usage & GpuBufferUsage::storageBuffer);
    return storageDesc;
}

void GpuBufferVk::write(std::span<const byte> src, u64 offset)
{
    if (src.size() == 0)
        return;

    HG_ASSERT(offset <= bufSize);
    HG_ASSERT(src.size() <= bufSize - offset);

    if (static_cast<u32>(memoryAccess) & static_cast<u32>(GpuMemoryHostAccess::write))
    {
        VkResult result = vmaCopyMemoryToAllocation(vk->vma, src.data(), alloc, offset, src.size());
        if (result != VK_SUCCESS)
            panic("Could not write gpu buffer: {}\n", vkResultToStr(result));
        return;
    }

    std::unique_ptr<GpuBuffer> stageBuffer = vk->createBuffer(
        src.size(), GpuBufferUsage::transferSrc, GpuMemoryUsage::stagingWrite);
    GpuBufferVk* stage = static_cast<GpuBufferVk*>(stageBuffer.get());
    stage->write(src, 0);

    std::unique_ptr<GpuCommandBuffer> cmd = vk->beginCommands();

    VkBufferCopy region{};
    region.dstOffset = offset;
    region.size = src.size();

    vkCmdCopyBuffer(
        static_cast<GpuCommandBufferVk*>(cmd.get())->cmd,
        stage->buffer,
        buffer,
        1,
        &region);

    vk->endCommands(std::move(cmd));

    lastState = GpuAccess::transferDst;
}

void GpuBufferVk::read(std::span<byte> dst, u64 offset) const
{
    HG_ASSERT(offset <= bufSize);
    HG_ASSERT(dst.size() <= bufSize - offset);
    if (dst.size() == 0)
        return;

    if (static_cast<u32>(memoryAccess) & static_cast<u32>(GpuMemoryHostAccess::read))
    {
        VkResult result = vmaCopyAllocationToMemory(vk->vma, alloc, offset, dst.data(), dst.size());
        if (result != VK_SUCCESS)
            panic("Could not read gpu buffer: {}\n", vkResultToStr(result));
        return;
    }

    std::unique_ptr<GpuBuffer> stageBuffer = vk->createBuffer(dst.size(),
        GpuBufferUsage::transferDst, GpuMemoryUsage::stagingRead);
    GpuBufferVk* stage = static_cast<GpuBufferVk*>(stageBuffer.get());

    std::unique_ptr<GpuCommandBuffer> cmd = vk->beginCommands();

    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = dst.size();

    vkCmdCopyBuffer(
        static_cast<GpuCommandBufferVk*>(cmd.get())->cmd,
        buffer,
        stage->buffer,
        1,
        &region);

    vk->endCommands(std::move(cmd));

    stage->read(dst, 0);

    lastState = GpuAccess::transferSrc;
}

GpuBufferVk::~GpuBufferVk() noexcept
{
    if (buffer != nullptr)
    {
        if (usage & GpuBufferUsage::storageBuffer)
            descriptorDestroy(vk, storageDesc, DescriptorType::storageBuffer);
        if (usage & GpuBufferUsage::uniformBuffer)
            descriptorDestroy(vk, uniformDesc, DescriptorType::uniformBuffer);
        vmaDestroyBuffer(vk->vma, buffer, alloc);
    }
}

Format GpuImageVk::format() const
{
    return imageFormat;
}

u32 GpuImageVk::width() const
{
    return imageWidth;
}

u32 GpuImageVk::height() const
{
    return imageHeight;
}

u32 GpuImageVk::depth() const
{
    return imageDepth;
}

u32 GpuImageVk::mipmapCount() const
{
    return mipLevels;
}

u32 GpuImageVk::layerCount() const
{
    return arrayLayers;
}

std::unique_ptr<GpuView> GpuImageVk::createView(GpuFilter filter)
{
    GpuViewCreateInfo config{};
    config.baseMipLevel = 0;
    config.levelCount = mipLevels;
    config.baseArrayLayer = 0;
    config.layerCount = arrayLayers;
    config.type = imageDimensionsToHgView(dimensions);
    config.filter = filter;
    config.edgeMode = GpuSamplerEdgeMode::repeat;
    config.border = GpuSamplerBorder::floatTransparentBlack;
    return createView(config);
}

std::unique_ptr<GpuView> GpuImageVk::createView(const GpuViewCreateInfo& config)
{
    HG_ASSERT(vk != nullptr);
    HG_ASSERT(config.levelCount > 0);
    HG_ASSERT(config.layerCount > 0);
    HG_ASSERT(config.baseMipLevel < mipLevels);
    HG_ASSERT(config.baseMipLevel + config.levelCount <= mipLevels);
    HG_ASSERT(config.baseArrayLayer < arrayLayers);
    HG_ASSERT(config.baseArrayLayer + config.layerCount <= arrayLayers);

    std::unique_ptr<GpuViewVk> view = std::make_unique<GpuViewVk>();
    view->vk = vk;
    view->image = this;

    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = gpuViewTypeToVk(config.type);
    info.format = formatToVk(imageFormat);
    info.subresourceRange.aspectMask = gpuFormatToAspect(imageFormat);
    info.subresourceRange.baseMipLevel = config.baseMipLevel;
    info.subresourceRange.levelCount = config.levelCount;
    info.subresourceRange.baseArrayLayer = config.baseArrayLayer;
    info.subresourceRange.layerCount = config.layerCount;

    [[maybe_unused]]
    VkResult result = vkCreateImageView(vk->device, &info, nullptr, &view->view);
    if (view->view == nullptr)
        panic("Could not create VkImageView: {}\n", vkResultToStr(result));

    if (usage & GpuImageUsage::sampled)
    {
        view->sampler = samplerGet(vk, config.filter, config.edgeMode, config.border);
        view->samplerDesc = createImageDescriptor(
            vk,
            DescriptorType::combinedImageSampler,
            view.get(),
            GpuLayout::shaderReadOnly);
    }

    if (usage & GpuImageUsage::storage)
    {
        view->storageDesc = createImageDescriptor(
            vk,
            DescriptorType::storageImage,
            view.get(),
            GpuLayout::general);
    }

    view->usage = usage;
    view->type = config.type;
    view->baseMipLevel = assertCast<u8>(config.baseMipLevel);
    view->levelCount = assertCast<u8>(config.levelCount);
    view->baseArrayLayer = assertCast<u8>(config.baseArrayLayer);
    view->arrayLayerCount = assertCast<u8>(config.layerCount);

    return view;
}

void GpuImageVk::write(std::span<const byte> src, const GpuImageRegion& dst)
{
    HG_ASSERT(vk != nullptr);
    HG_ASSERT(dst.width > 0 && dst.height > 0 && dst.depth > 0);
    HG_ASSERT(dst.mipLevel < mipLevels);
    HG_ASSERT(dst.arrayLayer + dst.layerCount <= arrayLayers);

    u32 mipW = imageWidth >> dst.mipLevel;
    u32 mipH = imageHeight >> dst.mipLevel;
    u32 mipD = imageDepth >> dst.mipLevel;
    if (mipW < 1) mipW = 1;
    if (mipH < 1) mipH = 1;
    if (mipD < 1) mipD = 1;
    HG_ASSERT(dst.x >= 0 && dst.y >= 0 && dst.z >= 0);
    HG_ASSERT(assertCast<u32>(dst.x) + dst.width <= mipW);
    HG_ASSERT(assertCast<u32>(dst.y) + dst.height <= mipH);
    HG_ASSERT(assertCast<u32>(dst.z) + dst.depth <= mipD);

    u64 writeSize = assertCast<u64>(dst.width)
                  * dst.height
                  * dst.depth
                  * dst.layerCount
                  * formatToSize(imageFormat);

    HG_ASSERT(src.size() == writeSize);

    std::unique_ptr<GpuBuffer> stageBuffer = vk->createBuffer(
        writeSize, GpuBufferUsage::transferSrc, GpuMemoryUsage::stagingWrite);
    GpuBufferVk* stage = static_cast<GpuBufferVk*>(stageBuffer.get());
    stage->write(src, 0);

    std::unique_ptr<GpuCommandBuffer> cmd = vk->beginCommands();
    VkCommandBuffer commandBuffer = static_cast<GpuCommandBufferVk*>(cmd.get())->cmd;

    VkImageAspectFlags aspect = gpuFormatToAspect(imageFormat);

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.srcStageMask = gpuStateToVkStage(lastState);
    transferBarrier.srcAccessMask = gpuStateToVkAccess(lastState);
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarrier.oldLayout = gpuLayoutToVk(lastLayout);
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = image;
    transferBarrier.subresourceRange = {
        aspect,
        dst.mipLevel,
        1,
        dst.arrayLayer,
        dst.layerCount,
    };

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(commandBuffer, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource = {aspect, dst.mipLevel, dst.arrayLayer, dst.layerCount};
    region.imageOffset = {dst.x, dst.y, dst.z};
    region.imageExtent = {dst.width, dst.height, dst.depth};

    vkCmdCopyBufferToImage(
        commandBuffer,
        stage->buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    vk->endCommands(std::move(cmd));

    lastState = GpuAccess::transferDst;
    lastLayout = GpuLayout::transferDst;
}

void GpuImageVk::writeWhole(std::span<const byte> src, u32 mipLevel)
{
    u32 mipW = imageWidth >> mipLevel;
    u32 mipH = imageHeight >> mipLevel;
    u32 mipD = imageDepth >> mipLevel;
    if (mipW < 1) mipW = 1;
    if (mipH < 1) mipH = 1;
    if (mipD < 1) mipD = 1;

    GpuImageRegion dst{};
    dst.width = mipW;
    dst.height = mipH;
    dst.depth = mipD;
    dst.mipLevel = mipLevel;
    dst.arrayLayer = 0;
    dst.layerCount = arrayLayers;

    write(src, dst);
}

void GpuImageVk::writeCubemap(std::span<const byte> src, u32 mipLevel)
{
    HG_ASSERT(vk != nullptr);
    HG_ASSERT(imageDepth == 1);
    HG_ASSERT(arrayLayers >= 6);

    u32 mipW = imageWidth >> mipLevel;
    u32 mipH = imageHeight >> mipLevel;
    if (mipW < 1) mipW = 1;
    if (mipH < 1) mipH = 1;

    u64 mipSize = assertCast<u64>(mipW) * mipH * formatToSize(imageFormat);

    HG_ASSERT(src.size() == mipSize * 4 * 3);

    std::unique_ptr<GpuBuffer> stageBuffer = vk->createBuffer(
        mipSize * 4 * 3, GpuBufferUsage::transferSrc, GpuMemoryUsage::stagingWrite);
    GpuBufferVk* buffer = static_cast<GpuBufferVk*>(stageBuffer.get());
    buffer->write(src, 0);

    std::unique_ptr<GpuImage> stageImage = vk->createImage(
        mipW * 4,
        mipH * 3,
        imageFormat,
        GpuImageUsage::transferDst | GpuImageUsage::transferSrc);
    GpuImageVk* stage = static_cast<GpuImageVk*>(stageImage.get());

    std::unique_ptr<GpuCommandBuffer> cmd = vk->beginCommands();
    VkCommandBuffer commandBuffer = static_cast<GpuCommandBufferVk*>(cmd.get())->cmd;

    VkImageAspectFlags aspect = gpuFormatToAspect(imageFormat);

    VkImageMemoryBarrier2 stageBarrier{};
    stageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    stageBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    stageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    stageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    stageBarrier.image = stage->image;
    stageBarrier.subresourceRange = {aspect, 0, 1, 0, 1};

    VkDependencyInfo stageDep{};
    stageDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    stageDep.imageMemoryBarrierCount = 1;
    stageDep.pImageMemoryBarriers = &stageBarrier;

    vkCmdPipelineBarrier2(commandBuffer, &stageDep);

    VkBufferImageCopy stageRegion{};
    stageRegion.imageSubresource = {aspect, 0, 0, 1};
    stageRegion.imageExtent = {mipW * 4, mipH * 3, 1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer->buffer,
        stage->image,
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
    transferBarriers[0].image = stage->image;
    transferBarriers[0].subresourceRange = {aspect, 0, 1, 0, 1};

    transferBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarriers[1].srcStageMask = gpuStateToVkStage(lastState);
    transferBarriers[1].srcAccessMask = gpuStateToVkAccess(lastState);
    transferBarriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarriers[1].oldLayout = gpuLayoutToVk(lastLayout);
    transferBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarriers[1].image = image;
    transferBarriers[1].subresourceRange = {aspect, mipLevel, 1, 0, 6};

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = assertCast<u32>(std::size(transferBarriers));
    transferDep.pImageMemoryBarriers = transferBarriers;

    vkCmdPipelineBarrier2(commandBuffer, &transferDep);

    VkImageCopy regions[6]{};

    regions[0].srcSubresource = {aspect, 0, 0, 1};
    regions[0].srcOffset = {assertCast<i32>(mipW) * 2, assertCast<i32>(mipH) * 1, 0};
    regions[0].dstSubresource = {aspect, mipLevel, 0, 1};
    regions[0].dstOffset = {};
    regions[0].extent = {mipW, mipH, 1};

    regions[1].srcSubresource = {aspect, 0, 0, 1};
    regions[1].srcOffset = {assertCast<i32>(mipW) * 0, assertCast<i32>(mipH) * 1, 0};
    regions[1].dstSubresource = {aspect, mipLevel, 1, 1};
    regions[1].dstOffset = {};
    regions[1].extent = {mipW, mipH, 1};

    regions[2].srcSubresource = {aspect, 0, 0, 1};
    regions[2].srcOffset = {assertCast<i32>(mipW) * 1, assertCast<i32>(mipH) * 2, 0};
    regions[2].dstSubresource = {aspect, mipLevel, 2, 1};
    regions[2].dstOffset = {};
    regions[2].extent = {mipW, mipH, 1};

    regions[3].srcSubresource = {aspect, 0, 0, 1};
    regions[3].srcOffset = {assertCast<i32>(mipW) * 1, assertCast<i32>(mipH) * 0, 0};
    regions[3].dstSubresource = {aspect, mipLevel, 3, 1};
    regions[3].dstOffset = {};
    regions[3].extent = {mipW, mipH, 1};

    regions[4].srcSubresource = {aspect, 0, 0, 1};
    regions[4].srcOffset = {assertCast<i32>(mipW) * 1, assertCast<i32>(mipH) * 1, 0};
    regions[4].dstSubresource = {aspect, mipLevel, 4, 1};
    regions[4].dstOffset = {};
    regions[4].extent = {mipW, mipH, 1};

    regions[5].srcSubresource = {aspect, 0, 0, 1};
    regions[5].srcOffset = {assertCast<i32>(mipW) * 3, assertCast<i32>(mipH) * 1, 0};
    regions[5].dstSubresource = {aspect, mipLevel, 5, 1};
    regions[5].dstOffset = {};
    regions[5].extent = {mipW, mipH, 1};

    vkCmdCopyImage(
        commandBuffer,
        stage->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        assertCast<u32>(std::size(regions)),
        regions);

    vk->endCommands(std::move(cmd));

    lastState = GpuAccess::transferDst;
    lastLayout = GpuLayout::transferDst;
}

void GpuImageVk::generateMipmaps(u32 arrayLayer)
{
    HG_ASSERT(vk != nullptr);
    if (mipLevels == 1)
        return;

    std::unique_ptr<GpuCommandBuffer> cmd = vk->beginCommands();
    VkCommandBuffer commandBuffer = static_cast<GpuCommandBufferVk*>(cmd.get())->cmd;

    VkImageAspectFlags aspect = gpuFormatToAspect(imageFormat);

    VkOffset3D mipOffset{};
    mipOffset.x = assertCast<i32>(imageWidth);
    mipOffset.y = assertCast<i32>(imageHeight);
    mipOffset.z = assertCast<i32>(imageDepth);

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = gpuLayoutToVk(lastLayout);
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = image;
    barrier.subresourceRange = {aspect, 0, 1, arrayLayer, 1};

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &dep);

    for (u32 level = 0; level < assertCast<u32>(mipLevels) - 1; ++level)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_NONE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.subresourceRange = {aspect, level + 1, 1, arrayLayer, 1};

        vkCmdPipelineBarrier2(commandBuffer, &dep);

        VkImageBlit blit{};
        blit.srcSubresource = {aspect, level, arrayLayer, 1};
        blit.srcOffsets[1] = mipOffset;
        if (mipOffset.x > 1)
            mipOffset.x /= 2;
        if (mipOffset.y > 1)
            mipOffset.y /= 2;
        if (mipOffset.z > 1)
            mipOffset.z /= 2;
        blit.dstSubresource = {aspect, level + 1, arrayLayer, 1};
        blit.dstOffsets[1] = mipOffset;

        vkCmdBlitImage(
            commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange = {aspect, level + 1, 1, arrayLayer, 1};

        vkCmdPipelineBarrier2(commandBuffer, &dep);
    }

    vk->endCommands(std::move(cmd));

    lastState = GpuAccess::transferSrc;
    lastLayout = GpuLayout::transferSrc;
}

void GpuImageVk::read(std::span<byte> dst, const GpuImageRegion& src)
{
    HG_ASSERT(vk != nullptr);
    HG_ASSERT(src.width > 0 && src.height > 0 && src.depth > 0);
    HG_ASSERT(src.mipLevel < mipLevels);
    HG_ASSERT(src.arrayLayer + src.layerCount <= arrayLayers);

    u32 mipW = imageWidth >> src.mipLevel;
    u32 mipH = imageHeight >> src.mipLevel;
    u32 mipD = imageDepth >> src.mipLevel;
    if (mipW < 1) mipW = 1;
    if (mipH < 1) mipH = 1;
    if (mipD < 1) mipD = 1;
    HG_ASSERT(src.x >= 0 && src.y >= 0 && src.z >= 0);
    HG_ASSERT(assertCast<u32>(src.x) + src.width <= mipW);
    HG_ASSERT(assertCast<u32>(src.y) + src.height <= mipH);
    HG_ASSERT(assertCast<u32>(src.z) + src.depth <= mipD);

    HG_ASSERT(dst.size() ==
        assertCast<u64>(src.width) *
        src.height *
        src.depth *
        src.layerCount *
        formatToSize(imageFormat));

    std::unique_ptr<GpuBuffer> stageBuffer = vk->createBuffer(
        dst.size(), GpuBufferUsage::transferDst, GpuMemoryUsage::stagingRead);
    GpuBufferVk* stage = static_cast<GpuBufferVk*>(stageBuffer.get());

    std::unique_ptr<GpuCommandBuffer> cmd = vk->beginCommands();
    VkCommandBuffer commandBuffer = static_cast<GpuCommandBufferVk*>(cmd.get())->cmd;

    VkImageAspectFlags aspect = gpuFormatToAspect(imageFormat);

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.srcStageMask = gpuStateToVkStage(lastState);
    transferBarrier.srcAccessMask = gpuStateToVkAccess(lastState);
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.oldLayout = gpuLayoutToVk(lastLayout);
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarrier.image = image;
    transferBarrier.subresourceRange = {
        aspect,
        src.mipLevel,
        1,
        src.arrayLayer,
        src.layerCount,
    };

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(commandBuffer, &transferDep);

    VkBufferImageCopy region{};
    region.imageSubresource = {aspect, src.mipLevel, src.arrayLayer, src.layerCount};
    region.imageOffset = {src.x, src.y, src.z};
    region.imageExtent = {src.width, src.height, src.depth};

    vkCmdCopyImageToBuffer(
        commandBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        stage->buffer,
        1,
        &region);

    vk->endCommands(std::move(cmd));

    stage->read(dst, 0);

    lastState = GpuAccess::transferSrc;
    lastLayout = GpuLayout::transferSrc;
}

void GpuImageVk::readWhole(std::span<byte> dst, u32 mipLevel)
{
    u32 mipW = imageWidth >> mipLevel;
    u32 mipH = imageHeight >> mipLevel;
    u32 mipD = imageDepth >> mipLevel;
    if (mipW < 1) mipW = 1;
    if (mipH < 1) mipH = 1;
    if (mipD < 1) mipD = 1;

    GpuImageRegion src{};
    src.width = mipW;
    src.height = mipH;
    src.depth = mipD;
    src.mipLevel = mipLevel;
    src.arrayLayer = 0;
    src.layerCount = arrayLayers;

    read(dst, src);
}

void GpuImageVk::readCubemap(std::span<byte> dst, u32 mipLevel)
{
    HG_ASSERT(vk != nullptr);
    HG_ASSERT(arrayLayers >= 6);

    u32 mipW = imageWidth >> mipLevel;
    u32 mipH = imageHeight >> mipLevel;
    if (mipW < 1) mipW = 1;
    if (mipH < 1) mipH = 1;

    u64 mipSize = assertCast<u64>(mipW) * mipH * formatToSize(imageFormat);

    HG_ASSERT(dst.size() == mipSize * 4 * 3);

    std::unique_ptr<GpuBuffer> stageBuffer = vk->createBuffer(
        mipSize * 4 * 3, GpuBufferUsage::transferDst, GpuMemoryUsage::stagingRead);
    GpuBufferVk* stage = static_cast<GpuBufferVk*>(stageBuffer.get());

    std::unique_ptr<GpuCommandBuffer> cmd = vk->beginCommands();
    VkCommandBuffer commandBuffer = static_cast<GpuCommandBufferVk*>(cmd.get())->cmd;

    VkImageAspectFlags aspect = gpuFormatToAspect(imageFormat);

    VkImageMemoryBarrier2 transferBarrier{};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.srcStageMask = gpuStateToVkStage(lastState);
    transferBarrier.srcAccessMask = gpuStateToVkAccess(lastState);
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.oldLayout = gpuLayoutToVk(lastLayout);
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    transferBarrier.image = image;
    transferBarrier.subresourceRange = {aspect, mipLevel, 1, 0, 6};

    VkDependencyInfo transferDep{};
    transferDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDep.imageMemoryBarrierCount = 1;
    transferDep.pImageMemoryBarriers = &transferBarrier;

    vkCmdPipelineBarrier2(commandBuffer, &transferDep);

    // Copy each face into its position in the cross layout, mirroring writeCubemap.
    static constexpr u32 dstX[6] = {2, 0, 1, 1, 1, 3};
    static constexpr u32 dstY[6] = {1, 1, 2, 0, 1, 1};

    VkBufferImageCopy regions[6]{};
    for (u32 i = 0; i < 6; ++i)
    {
        regions[i].bufferRowLength = mipW * 4;
        regions[i].bufferImageHeight = mipH * 3;
        regions[i].imageSubresource = {aspect, mipLevel, i, 1};
        regions[i].imageOffset = {
            assertCast<i32>(dstX[i] * mipW),
            assertCast<i32>(dstY[i] * mipH),
            0};
        regions[i].imageExtent = {mipW, mipH, 1};
    }

    vkCmdCopyImageToBuffer(
        commandBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        stage->buffer,
        assertCast<u32>(std::size(regions)),
        regions);

    vk->endCommands(std::move(cmd));

    stage->read(dst, 0);

    lastState = GpuAccess::transferSrc;
    lastLayout = GpuLayout::transferSrc;
}

GpuImageVk::~GpuImageVk() noexcept
{
    if (image != nullptr)
    {
        vmaDestroyImage(vk->vma, image, alloc);
    }
}

Format GpuViewVk::format() const
{
    return image->imageFormat;
}

u32 GpuViewVk::width() const
{
    return image->imageWidth;
}

u32 GpuViewVk::height() const
{
    return image->imageHeight;
}

u32 GpuViewVk::depth() const
{
    return image->imageDepth;
}

u32 GpuViewVk::mipmapCount() const
{
    return levelCount;
}

u32 GpuViewVk::layerCount() const
{
    return arrayLayerCount;
}

u32 GpuViewVk::samplerDescriptor() const
{
    HG_ASSERT(usage & GpuImageUsage::sampled);
    return samplerDesc;
}

u32 GpuViewVk::storageDescriptor() const
{
    HG_ASSERT(usage & GpuImageUsage::storage);
    return storageDesc;
}

GpuViewVk::~GpuViewVk() noexcept
{
    if (view != nullptr)
    {
        if (usage & GpuImageUsage::storage)
            descriptorDestroy(vk, storageDesc, DescriptorType::storageImage);
        if (usage & GpuImageUsage::sampled)
            descriptorDestroy(vk, samplerDesc, DescriptorType::combinedImageSampler);
        vkDestroyImageView(vk->device, view, nullptr);
    }
}

GpuPipelineVk::~GpuPipelineVk() noexcept
{
    if (pipeline != nullptr)
    {
        vkDestroyPipeline(vk->device, pipeline, nullptr);
        vkDestroyPipelineLayout(vk->device, layout, nullptr);
    }
}

void GpuCommandBufferVk::memoryBarrier(
    std::span<const GpuBufferBarrier> bufferBarriers,
    std::span<const GpuImageBarrier> imageBarriers)
{
    HG_ASSERT(cmd != nullptr);

    ArenaScope scratch = getScratch();

    VectorTemp<VkBufferMemoryBarrier2> vkBufferBarriers{scratch};
    vkBufferBarriers.reserve(assertCast<u32>(bufferBarriers.size()));
    VectorTemp<VkImageMemoryBarrier2> vkImageBarriers{scratch};
    vkImageBarriers.reserve(assertCast<u32>(imageBarriers.size()));

    for (const GpuBufferBarrier& barrier : bufferBarriers)
    {
        VkBufferMemoryBarrier2& vkBarrier = vkBufferBarriers.emplace_back();
        GpuBufferVk* buffer = static_cast<GpuBufferVk*>(barrier.buffer);

        vkBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        vkBarrier.srcStageMask = gpuStateToVkStage(buffer->lastState);
        vkBarrier.srcAccessMask = gpuStateToVkAccess(buffer->lastState);
        vkBarrier.dstStageMask = gpuStateToVkStage(barrier.nextAccess);
        vkBarrier.dstAccessMask = gpuStateToVkAccess(barrier.nextAccess);
        vkBarrier.buffer = buffer->buffer;
        vkBarrier.size = buffer->bufSize;

        buffer->lastState = barrier.nextAccess;
    }

    for (const GpuImageBarrier& barrier : imageBarriers)
    {
        VkImageMemoryBarrier2& vkBarrier = vkImageBarriers.emplace_back();
        GpuViewVk* view = static_cast<GpuViewVk*>(barrier.image);

        vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        vkBarrier.srcStageMask = gpuStateToVkStage(view->image->lastState);
        vkBarrier.srcAccessMask = gpuStateToVkAccess(view->image->lastState);
        vkBarrier.dstStageMask = gpuStateToVkStage(barrier.nextAccess);
        vkBarrier.dstAccessMask = gpuStateToVkAccess(barrier.nextAccess);
        vkBarrier.oldLayout = gpuLayoutToVk(view->image->lastLayout);
        vkBarrier.newLayout = gpuLayoutToVk(barrier.nextLayout);
        vkBarrier.image = view->image->image;
        vkBarrier.subresourceRange = {
            gpuFormatToAspect(view->image->imageFormat),
            view->baseMipLevel,
            view->levelCount,
            view->baseArrayLayer,
            view->arrayLayerCount,
        };

        view->image->lastState = barrier.nextAccess;
        view->image->lastLayout = barrier.nextLayout;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = assertCast<u32>(vkBufferBarriers.size());
    dep.pBufferMemoryBarriers = vkBufferBarriers.data();
    dep.imageMemoryBarrierCount = assertCast<u32>(vkImageBarriers.size());
    dep.pImageMemoryBarriers = vkImageBarriers.data();

    vkCmdPipelineBarrier2(cmd, &dep);
}

void GpuCommandBufferVk::copyBuffer(
    const GpuBuffer* dst,
    usize dstOffset,
    const GpuBuffer* src,
    usize srcOffset,
    usize size)
{
    HG_ASSERT(cmd != nullptr);
    HG_ASSERT(src != nullptr);
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(size > 0);

    GpuBufferVk* dstBuffer = static_cast<GpuBufferVk*>(const_cast<GpuBuffer*>(dst));
    GpuBufferVk* srcBuffer = static_cast<GpuBufferVk*>(const_cast<GpuBuffer*>(src));

    HG_ASSERT(srcOffset + size <= srcBuffer->bufSize);
    HG_ASSERT(dstOffset + size <= dstBuffer->bufSize);

    VkBufferMemoryBarrier2 barriers[2]{};

    barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barriers[0].srcStageMask = gpuStateToVkStage(srcBuffer->lastState);
    barriers[0].srcAccessMask = gpuStateToVkAccess(srcBuffer->lastState);
    barriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].buffer = srcBuffer->buffer;
    barriers[0].size = srcBuffer->bufSize;

    barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].buffer = dstBuffer->buffer;
    barriers[1].size = dstBuffer->bufSize;

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = assertCast<u32>(std::size(barriers));
    dep.pBufferMemoryBarriers = barriers;

    vkCmdPipelineBarrier2(cmd, &dep);

    VkBufferCopy region{};
    region.srcOffset = srcOffset;
    region.dstOffset = dstOffset;
    region.size = size;

    vkCmdCopyBuffer(cmd, srcBuffer->buffer, dstBuffer->buffer, 1, &region);

    srcBuffer->lastState = GpuAccess::transferSrc;
    dstBuffer->lastState = GpuAccess::transferDst;
}

void GpuCommandBufferVk::copyImage(
    const GpuImage* dst,
    const GpuImageRegion& dstRegion,
    const GpuImage* src,
    const GpuImageRegion& srcRegion)
{
    HG_ASSERT(cmd != nullptr);
    HG_ASSERT(src != nullptr);
    HG_ASSERT(dst != nullptr);

    GpuImageVk* dstImage = static_cast<GpuImageVk*>(const_cast<GpuImage*>(dst));
    GpuImageVk* srcImage = static_cast<GpuImageVk*>(const_cast<GpuImage*>(src));

    HG_ASSERT(srcImage->lastLayout != GpuLayout::undefined);
    HG_ASSERT(srcImage->imageFormat == dstImage->imageFormat);
    HG_ASSERT(srcRegion.width == dstRegion.width);
    HG_ASSERT(srcRegion.height == dstRegion.height);
    HG_ASSERT(srcRegion.depth == dstRegion.depth);

    VkImageAspectFlags srcAspect = gpuFormatToAspect(srcImage->imageFormat);
    VkImageAspectFlags dstAspect = gpuFormatToAspect(dstImage->imageFormat);

    VkImageMemoryBarrier2 barriers[2]{};

    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[0].srcStageMask = gpuStateToVkStage(srcImage->lastState);
    barriers[0].srcAccessMask = gpuStateToVkAccess(srcImage->lastState);
    barriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].oldLayout = gpuLayoutToVk(srcImage->lastLayout);
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].image = srcImage->image;
    barriers[0].subresourceRange = {
        srcAspect,
        srcRegion.mipLevel,
        1,
        srcRegion.arrayLayer,
        srcRegion.layerCount,
    };

    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].image = dstImage->image;
    barriers[1].subresourceRange = {
        dstAspect,
        dstRegion.mipLevel,
        1,
        dstRegion.arrayLayer,
        dstRegion.layerCount,
    };

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = assertCast<u32>(std::size(barriers));
    dep.pImageMemoryBarriers = barriers;

    vkCmdPipelineBarrier2(cmd, &dep);

    VkImageCopy region{};
    region.srcSubresource = {srcAspect, srcRegion.mipLevel, srcRegion.arrayLayer, srcRegion.layerCount};
    region.srcOffset = {srcRegion.x, srcRegion.y, srcRegion.z};
    region.dstSubresource = {dstAspect, dstRegion.mipLevel, dstRegion.arrayLayer, dstRegion.layerCount};
    region.dstOffset = {dstRegion.x, dstRegion.y, dstRegion.z};
    region.extent = {dstRegion.width, dstRegion.height, dstRegion.depth};

    vkCmdCopyImage(
        cmd,
        srcImage->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    srcImage->lastState = GpuAccess::transferSrc;
    srcImage->lastLayout = GpuLayout::transferSrc;
    dstImage->lastState = GpuAccess::transferDst;
    dstImage->lastLayout = GpuLayout::transferDst;
}

void GpuCommandBufferVk::copyImageWhole(const GpuImage* dst, const GpuImage* src)
{
    HG_ASSERT(cmd != nullptr);
    HG_ASSERT(src != nullptr);
    HG_ASSERT(dst != nullptr);

    GpuImageVk* dstImage = static_cast<GpuImageVk*>(const_cast<GpuImage*>(dst));
    GpuImageVk* srcImage = static_cast<GpuImageVk*>(const_cast<GpuImage*>(src));

    HG_ASSERT(srcImage->lastLayout != GpuLayout::undefined);
    HG_ASSERT(srcImage->imageFormat == dstImage->imageFormat);
    HG_ASSERT(srcImage->imageWidth == dstImage->imageWidth);
    HG_ASSERT(srcImage->imageHeight == dstImage->imageHeight);
    HG_ASSERT(srcImage->arrayLayers == dstImage->arrayLayers);

    VkImageAspectFlags aspect = gpuFormatToAspect(srcImage->imageFormat);

    VkImageMemoryBarrier2 barriers[2]{};

    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[0].srcStageMask = gpuStateToVkStage(srcImage->lastState);
    barriers[0].srcAccessMask = gpuStateToVkAccess(srcImage->lastState);
    barriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].oldLayout = gpuLayoutToVk(srcImage->lastLayout);
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].image = srcImage->image;
    barriers[0].subresourceRange = {aspect, 0, 1, 0, srcImage->arrayLayers};

    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].image = dstImage->image;
    barriers[1].subresourceRange = {aspect, 0, 1, 0, dstImage->arrayLayers};

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 2;
    dep.pImageMemoryBarriers = barriers;

    vkCmdPipelineBarrier2(cmd, &dep);

    VkImageCopy region{};
    region.srcSubresource = {aspect, 0, 0, srcImage->arrayLayers};
    region.dstSubresource = {aspect, 0, 0, dstImage->arrayLayers};
    region.extent = {srcImage->imageWidth, srcImage->imageHeight, srcImage->imageDepth};

    vkCmdCopyImage(
        cmd,
        srcImage->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    srcImage->lastState = GpuAccess::transferSrc;
    srcImage->lastLayout = GpuLayout::transferSrc;
    dstImage->lastState = GpuAccess::transferDst;
    dstImage->lastLayout = GpuLayout::transferDst;
}

void GpuCommandBufferVk::blitImage(
    const GpuImage* dst,
    const GpuImageRegion& dstRegion,
    const GpuImage* src,
    const GpuImageRegion& srcRegion)
{
    HG_ASSERT(cmd != nullptr);
    HG_ASSERT(src != nullptr);
    HG_ASSERT(dst != nullptr);

    GpuImageVk* dstImage = static_cast<GpuImageVk*>(const_cast<GpuImage*>(dst));
    GpuImageVk* srcImage = static_cast<GpuImageVk*>(const_cast<GpuImage*>(src));

    HG_ASSERT(srcImage->lastLayout != GpuLayout::undefined);
    HG_ASSERT(srcRegion.width > 0 && srcRegion.height > 0 && srcRegion.depth > 0);
    HG_ASSERT(dstRegion.width > 0 && dstRegion.height > 0 && dstRegion.depth > 0);

    VkImageAspectFlags srcAspect = gpuFormatToAspect(srcImage->imageFormat);
    VkImageAspectFlags dstAspect = gpuFormatToAspect(dstImage->imageFormat);

    VkImageMemoryBarrier2 barriers[2]{};

    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[0].srcStageMask = gpuStateToVkStage(srcImage->lastState);
    barriers[0].srcAccessMask = gpuStateToVkAccess(srcImage->lastState);
    barriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].oldLayout = gpuLayoutToVk(srcImage->lastLayout);
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].image = srcImage->image;
    barriers[0].subresourceRange = {
        srcAspect,
        srcRegion.mipLevel,
        1,
        srcRegion.arrayLayer,
        srcRegion.layerCount,
    };

    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].image = dstImage->image;
    barriers[1].subresourceRange = {
        dstAspect,
        dstRegion.mipLevel,
        1,
        dstRegion.arrayLayer,
        dstRegion.layerCount,
    };

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = assertCast<u32>(std::size(barriers));
    dep.pImageMemoryBarriers = barriers;

    vkCmdPipelineBarrier2(cmd, &dep);

    VkImageBlit region{};
    region.srcSubresource = {srcAspect, srcRegion.mipLevel, srcRegion.arrayLayer, srcRegion.layerCount};
    region.srcOffsets[0] = {srcRegion.x, srcRegion.y, srcRegion.z};
    region.srcOffsets[1] = {
        srcRegion.x + assertCast<i32>(srcRegion.width),
        srcRegion.y + assertCast<i32>(srcRegion.height),
        srcRegion.z + assertCast<i32>(srcRegion.depth)};
    region.dstSubresource = {dstAspect, dstRegion.mipLevel, dstRegion.arrayLayer, dstRegion.layerCount};
    region.dstOffsets[0] = {dstRegion.x, dstRegion.y, dstRegion.z};
    region.dstOffsets[1] = {
        dstRegion.x + assertCast<i32>(dstRegion.width),
        dstRegion.y + assertCast<i32>(dstRegion.height),
        dstRegion.z + assertCast<i32>(dstRegion.depth)};

    vkCmdBlitImage(
        cmd,
        srcImage->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region,
        VK_FILTER_LINEAR);

    srcImage->lastState = GpuAccess::transferSrc;
    srcImage->lastLayout = GpuLayout::transferSrc;
    dstImage->lastState = GpuAccess::transferDst;
    dstImage->lastLayout = GpuLayout::transferDst;
}

void GpuCommandBufferVk::blitImageWhole(const GpuImage* dst, const GpuImage* src)
{
    HG_ASSERT(cmd != nullptr);
    HG_ASSERT(src != nullptr);
    HG_ASSERT(dst != nullptr);

    const GpuImageVk* dstImage = static_cast<const GpuImageVk*>(dst);
    const GpuImageVk* srcImage = static_cast<const GpuImageVk*>(src);

    HG_ASSERT(srcImage->lastLayout != GpuLayout::undefined);

    VkImageAspectFlags srcAspect = gpuFormatToAspect(srcImage->imageFormat);
    VkImageAspectFlags dstAspect = gpuFormatToAspect(dstImage->imageFormat);

    VkImageMemoryBarrier2 barriers[2]{};

    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[0].srcStageMask = gpuStateToVkStage(srcImage->lastState);
    barriers[0].srcAccessMask = gpuStateToVkAccess(srcImage->lastState);
    barriers[0].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].oldLayout = gpuLayoutToVk(srcImage->lastLayout);
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].image = srcImage->image;
    barriers[0].subresourceRange = {
        srcAspect,
        0, srcImage->mipLevels,
        0, srcImage->arrayLayers,
    };

    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].image = dstImage->image;
    barriers[1].subresourceRange = {
        dstAspect,
        0, dstImage->mipLevels,
        0, dstImage->arrayLayers,
    };

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = assertCast<u32>(std::size(barriers));
    dep.pImageMemoryBarriers = barriers;

    vkCmdPipelineBarrier2(cmd, &dep);

    VkImageBlit region{};
    region.srcSubresource = {
        srcAspect,
        0,
        0,
        srcImage->arrayLayers};
    region.srcOffsets[1] = {
        assertCast<i32>(srcImage->imageWidth),
        assertCast<i32>(srcImage->imageHeight),
        assertCast<i32>(srcImage->imageDepth)};
    region.dstSubresource = {
        dstAspect,
        0,
        0,
        dstImage->arrayLayers};
    region.dstOffsets[1] = {
        assertCast<i32>(dstImage->imageWidth),
        assertCast<i32>(dstImage->imageHeight),
        assertCast<i32>(dstImage->imageDepth)};

    vkCmdBlitImage(
        cmd,
        srcImage->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region,
        VK_FILTER_LINEAR);

    srcImage->lastState = GpuAccess::transferSrc;
    srcImage->lastLayout = GpuLayout::transferSrc;
    dstImage->lastState = GpuAccess::transferDst;
    dstImage->lastLayout = GpuLayout::transferDst;
}

void GpuCommandBufferVk::computePass(const GpuPass& pass)
{
    HG_ASSERT(cmd != nullptr);

    ArenaScope scratch = getScratch();

    VectorTemp<VkBufferMemoryBarrier2> bufferBarriers{scratch};
    bufferBarriers.reserve(32);
    VectorTemp<VkImageMemoryBarrier2> imageBarriers{scratch};
    imageBarriers.reserve(32);

    for (u32 i = 0; i < pass.uniformBuffers.size(); ++i)
    {
        VkBufferMemoryBarrier2& barrier = bufferBarriers.emplace_back();
        GpuBufferVk* buffer = static_cast<GpuBufferVk*>(pass.uniformBuffers[i]);

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(buffer->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(buffer->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        barrier.buffer = buffer->buffer;
        barrier.size = buffer->bufSize;

        buffer->lastState = GpuAccess::uniformBufferCompute;
    }

    for (u32 i = 0; i < pass.storageBuffers.size(); ++i)
    {
        VkBufferMemoryBarrier2& barrier = bufferBarriers.emplace_back();
        GpuBufferVk* buffer = static_cast<GpuBufferVk*>(pass.storageBuffers[i]);

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(buffer->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(buffer->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.buffer = buffer->buffer;
        barrier.size = buffer->bufSize;

        buffer->lastState = GpuAccess::storageBufferCompute;
    }

    for (u32 i = 0; i < pass.sampledImages.size(); ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.emplace_back();
        GpuViewVk* image = static_cast<GpuViewVk*>(pass.sampledImages[i]);

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(image->image->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(image->image->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = gpuLayoutToVk(image->image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuFormatToAspect(image->image->imageFormat),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->arrayLayerCount,
        };

        image->image->lastState = GpuAccess::sampledImageCompute;
        image->image->lastLayout = GpuLayout::shaderReadOnly;
    }

    for (u32 i = 0; i < pass.storageImages.size(); ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.emplace_back();
        GpuViewVk* image = static_cast<GpuViewVk*>(pass.storageImages[i]);

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(image->image->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(image->image->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = gpuLayoutToVk(image->image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuFormatToAspect(image->image->imageFormat),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->arrayLayerCount,
        };

        image->image->lastState = GpuAccess::storageImageCompute;
        image->image->lastLayout = GpuLayout::general;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = assertCast<u32>(bufferBarriers.size());
    dep.pBufferMemoryBarriers = bufferBarriers.data();
    dep.imageMemoryBarrierCount = assertCast<u32>(imageBarriers.size());
    dep.pImageMemoryBarriers = imageBarriers.data();

    vkCmdPipelineBarrier2(cmd, &dep);
}

void GpuCommandBufferVk::beginRenderPass(const GpuPass& pass)
{
    HG_ASSERT(cmd != nullptr);

    ArenaScope scratch = getScratch();

    VectorTemp<VkBufferMemoryBarrier2> bufferBarriers{scratch};
    bufferBarriers.reserve(32);
    VectorTemp<VkImageMemoryBarrier2> imageBarriers{scratch};
    imageBarriers.reserve(32);

    for (u32 i = 0; i < pass.uniformBuffers.size(); ++i)
    {
        VkBufferMemoryBarrier2& barrier = bufferBarriers.emplace_back();
        GpuBufferVk* buffer = static_cast<GpuBufferVk*>(pass.uniformBuffers[i]);

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(buffer->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(buffer->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        barrier.buffer = buffer->buffer;
        barrier.size = buffer->bufSize;

        buffer->lastState = GpuAccess::uniformBufferAllGraphics;
    }

    for (u32 i = 0; i < pass.storageBuffers.size(); ++i)
    {
        VkBufferMemoryBarrier2& barrier = bufferBarriers.emplace_back();
        GpuBufferVk* buffer = static_cast<GpuBufferVk*>(pass.storageBuffers[i]);

        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(buffer->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(buffer->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.buffer = buffer->buffer;
        barrier.size = buffer->bufSize;

        buffer->lastState = GpuAccess::storageBufferAllGraphics;
    }

    for (u32 i = 0; i < pass.sampledImages.size(); ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.emplace_back();
        GpuViewVk* image = static_cast<GpuViewVk*>(pass.sampledImages[i]);

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(image->image->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(image->image->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = gpuLayoutToVk(image->image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuFormatToAspect(image->image->imageFormat),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->arrayLayerCount,
        };

        image->image->lastState = GpuAccess::sampledImageFragment;
        image->image->lastLayout = GpuLayout::shaderReadOnly;
    }

    for (u32 i = 0; i < pass.storageImages.size(); ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.emplace_back();
        GpuViewVk* image = static_cast<GpuViewVk*>(pass.storageImages[i]);

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(image->image->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(image->image->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = gpuLayoutToVk(image->image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuFormatToAspect(image->image->imageFormat),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->arrayLayerCount,
        };

        image->image->lastState = GpuAccess::storageImageFragment;
        image->image->lastLayout = GpuLayout::general;
    }

    for (u32 i = 0; i < pass.colorAttachments.size(); ++i)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.emplace_back();
        GpuViewVk* image = static_cast<GpuViewVk*>(pass.colorAttachments[i].image);

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(image->image->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(image->image->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (pass.colorAttachments[i].loadOp == GpuLoadOp::load)
            barrier.oldLayout = gpuLayoutToVk(image->image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuFormatToAspect(image->image->imageFormat),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->arrayLayerCount,
        };

        image->image->lastState = GpuAccess::colorAttachment;
        image->image->lastLayout = GpuLayout::colorAttachment;
    }

    if (pass.depthAttachment != nullptr)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.emplace_back();
        GpuViewVk* image = static_cast<GpuViewVk*>(pass.depthAttachment->image);

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(image->image->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(image->image->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                              | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (pass.depthAttachment->loadOp == GpuLoadOp::load)
            barrier.oldLayout = gpuLayoutToVk(image->image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuFormatToAspect(image->image->imageFormat),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->arrayLayerCount,
        };

        image->image->lastState = GpuAccess::depthStencilAttachment;
        image->image->lastLayout = GpuLayout::depthStencilAttachment;
    }

    if (pass.stencilAttachment != nullptr)
    {
        VkImageMemoryBarrier2& barrier = imageBarriers.emplace_back();
        GpuViewVk* image = static_cast<GpuViewVk*>(pass.stencilAttachment->image);

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = gpuStateToVkStage(image->image->lastState);
        barrier.srcAccessMask = gpuStateToVkAccess(image->image->lastState);
        barrier.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                              | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                               | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if (pass.stencilAttachment->loadOp == GpuLoadOp::load)
            barrier.oldLayout = gpuLayoutToVk(image->image->lastLayout);
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.image = image->image->image;
        barrier.subresourceRange = {
            gpuFormatToAspect(image->image->imageFormat),
            image->baseMipLevel,
            image->levelCount,
            image->baseArrayLayer,
            image->arrayLayerCount,
        };

        image->image->lastState = GpuAccess::depthStencilAttachment;
        image->image->lastLayout = GpuLayout::depthStencilAttachment;
    }

    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = assertCast<u32>(bufferBarriers.size());
    dep.pBufferMemoryBarriers = bufferBarriers.data();
    dep.imageMemoryBarrierCount = assertCast<u32>(imageBarriers.size());
    dep.pImageMemoryBarriers = imageBarriers.data();

    vkCmdPipelineBarrier2(cmd, &dep);

    VectorTemp<VkRenderingAttachmentInfo> colorAttachments{scratch};
    colorAttachments.reserve(assertCast<u32>(pass.colorAttachments.size()));

    for (u32 i = 0; i < pass.colorAttachments.size(); ++i)
    {
        GpuViewVk* view = static_cast<GpuViewVk*>(pass.colorAttachments[i].image);
        VkRenderingAttachmentInfo& attachment = colorAttachments.emplace_back();
        attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachment.imageView = view->view;
        attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.loadOp = gpuLoadOpToVk(pass.colorAttachments[i].loadOp);
        attachment.storeOp = gpuStoreOpToVk(pass.colorAttachments[i].storeOp);
        std::memcpy(&attachment.clearValue, &pass.colorAttachments[i].clearColor, sizeof(VkClearValue));
    }

    VkRenderingAttachmentInfo depthAttachment{};
    if (pass.depthAttachment != nullptr)
    {
        GpuViewVk* view = static_cast<GpuViewVk*>(pass.depthAttachment->image);
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = view->view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = gpuLoadOpToVk(pass.depthAttachment->loadOp);
        depthAttachment.storeOp = gpuStoreOpToVk(pass.depthAttachment->storeOp);
        std::memcpy(&depthAttachment.clearValue, &pass.depthAttachment->clearColor, sizeof(VkClearValue));
    }

    VkRenderingAttachmentInfo stencilAttachment{};
    if (pass.stencilAttachment != nullptr)
    {
        GpuViewVk* view = static_cast<GpuViewVk*>(pass.stencilAttachment->image);
        stencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        stencilAttachment.imageView = view->view;
        stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencilAttachment.loadOp = gpuLoadOpToVk(pass.stencilAttachment->loadOp);
        stencilAttachment.storeOp = gpuStoreOpToVk(pass.stencilAttachment->storeOp);
        std::memcpy(&stencilAttachment.clearValue, &pass.stencilAttachment->clearColor, sizeof(VkClearValue));
    }

    u32 width, height;
    if (pass.colorAttachments.size() > 0)
    {
        GpuViewVk* view = static_cast<GpuViewVk*>(pass.colorAttachments[0].image);
        width = view->image->imageWidth;
        height = view->image->imageHeight;
    }
    else if (pass.depthAttachment != nullptr)
    {
        GpuViewVk* view = static_cast<GpuViewVk*>(pass.depthAttachment->image);
        width = view->image->imageWidth;
        height = view->image->imageHeight;
    }
    else
    {
        HG_ASSERT(pass.stencilAttachment != nullptr);
        GpuViewVk* view = static_cast<GpuViewVk*>(pass.stencilAttachment->image);
        width = view->image->imageWidth;
        height = view->image->imageHeight;
    }

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = pass.layerCount;
    renderingInfo.colorAttachmentCount = assertCast<u32>(pass.colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = pass.depthAttachment != nullptr
        ? &depthAttachment
        : nullptr;
    renderingInfo.pStencilAttachment = pass.stencilAttachment != nullptr
        ? &stencilAttachment
        : nullptr;

    vkCmdBeginRendering(cmd, &renderingInfo);

    gpuSetViewport(0, 0, assertCast<f32>(width), assertCast<f32>(height));
    gpuSetScissor(0, 0, width, height);
}

void GpuCommandBufferVk::endRenderPass()
{
    vkCmdEndRendering(cmd);
}

void GpuCommandBufferVk::gpuSetViewport(f32 x, f32 y, f32 width, f32 height, f32 near, f32 far)
{
    VkViewport viewport{x, y, width, height, near, far};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
}

void GpuCommandBufferVk::gpuSetScissor(i32 x, i32 y, u32 width, u32 height)
{
    VkRect2D scissor{{x, y}, {width, height}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void GpuCommandBufferVk::bindPipeline(const GpuPipeline* pipeline)
{
    HG_ASSERT(cmd != nullptr);

    const GpuPipelineVk* pipe = static_cast<const GpuPipelineVk*>(pipeline);

    vkCmdBindPipeline(cmd, pipe->bindPoint, pipe->pipeline);

    vkCmdBindDescriptorSets(
        cmd,
        pipe->bindPoint,
        pipe->layout,
        0,
        1,
        &vk->bindlessSet,
        0,
        nullptr);
}

void GpuCommandBufferVk::pushConstants(const GpuPipeline* pipeline, std::span<const byte> push)
{
    const GpuPipelineVk* pipe = static_cast<const GpuPipelineVk*>(pipeline);
    HG_ASSERT(push.size() <= pipe->pushSize);

    vkCmdPushConstants(
        cmd,
        pipe->layout,
        pipe->bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE
            ? VK_SHADER_STAGE_COMPUTE_BIT
            : VK_SHADER_STAGE_ALL,
        0,
        assertCast<u32>(push.size()),
        push.data());
}

void GpuCommandBufferVk::draw(u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount)
{
    vkCmdDraw(cmd, vertexCount, instanceCount, vertexBegin, instanceBegin);
}

void GpuCommandBufferVk::dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ)
{
    vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
}

GpuCommandBufferVk::~GpuCommandBufferVk() noexcept
{
    HG_ASSERT(cmd == nullptr);
}

void GpuSwapchainVk::resize(u32 newWidth, u32 newHeight)
{
    HG_ASSERT(vk != nullptr);

    ArenaScope scratch = getScratch();

    vkQueueWaitIdle(vk->queue);

    for (u32 i = 0; i < views.size(); ++i)
    {
        if (views[i].view != nullptr)
        {
            vkDestroyImageView(vk->device, views[i].view, nullptr);
            views[i].view = nullptr;
        }

        if (i < readyToPresent.size() && readyToPresent[i] != nullptr)
        {
            vkDestroySemaphore(vk->device, readyToPresent[i], nullptr);
            readyToPresent[i] = nullptr;
        }
    }

    for (u32 i = 0; i < imageAvailable.size(); ++i)
    {
        if (imageAvailable[i] != nullptr)
        {
            vkDestroySemaphore(vk->device, imageAvailable[i], nullptr);
            imageAvailable[i] = nullptr;
        }
    }

    for (GpuImageVk& image : images)
        image.image = nullptr;

    VkSwapchainKHR oldSwapchain = swapchain;

    imageWidth = newWidth;
    imageHeight = newHeight;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vk->physicalDevice, surface, &capabilities);

    if (capabilities.currentExtent.width != (u32)-1)
        imageWidth = capabilities.currentExtent.width;
    if (capabilities.currentExtent.height != (u32)-1)
        imageHeight = capabilities.currentExtent.height;

    if (imageWidth != 0 && imageHeight != 0)
    {
        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount =
            std::min(capabilities.minImageCount, capabilities.maxImageCount - 1) + 1;
        swapchainInfo.imageFormat = formatToVk(imageFormat);
        swapchainInfo.imageExtent = {imageWidth, imageHeight};
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = gpuImageUsageToVk(imageUsage);
        swapchainInfo.preTransform = capabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentModeToVk(presentMode);
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = oldSwapchain;

        [[maybe_unused]]
        VkResult result = vkCreateSwapchainKHR(
            vk->device, &swapchainInfo, nullptr, &swapchain);
        if (swapchain == nullptr)
            panic("Failed to create swapchain: {}\n",
                vkResultToStr(result));

        u32 swapImageCount;
        vkGetSwapchainImagesKHR(
            vk->device, swapchain, &swapImageCount, nullptr);

        if (images.size() != swapImageCount)
        {
            images.resize(swapImageCount);
            views.resize(swapImageCount);
            readyToPresent.resize(swapImageCount);
        }

        VectorTemp<VkImage> swapImages{scratch};
        swapImages.resize(swapImageCount);
        vkGetSwapchainImagesKHR(
            vk->device, swapchain, &swapImageCount, swapImages.data());

        for (u32 i = 0; i < images.size(); ++i)
        {
            GpuImageVk& image = images[i];
            image.vk = vk;
            image.image = swapImages[i];
            image.alloc = nullptr;
            image.usage = imageUsage;
            image.imageFormat = imageFormat;
            image.imageWidth = imageWidth;
            image.imageHeight = imageHeight;
            image.imageDepth = 1;
            image.dimensions = 2;
            image.mipLevels = 1;
            image.arrayLayers = 1;
            image.msaaSamples = 1;
            image.lastState = GpuAccess::transferDst;
            image.lastLayout = GpuLayout::undefined;

            GpuViewVk& view = views[i];
            view.vk = vk;
            view.image = &image;
            view.usage = 0;
            view.type = GpuViewType::image2D;
            view.baseMipLevel = 0;
            view.levelCount = 1;
            view.baseArrayLayer = 0;
            view.arrayLayerCount = 1;
            view.image->lastState = GpuAccess::transferDst;
            view.image->lastLayout = GpuLayout::undefined;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = formatToVk(imageFormat);
            viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

            [[maybe_unused]]
            VkResult viewResult = vkCreateImageView(
                vk->device, &viewInfo, nullptr, &view.view);
            if (view.view == nullptr)
                panic("Could not create VkImageView: {}\n",
                    vkResultToStr(viewResult));

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            [[maybe_unused]]
            VkResult readyResult = vkCreateSemaphore(
                vk->device, &semaphoreInfo, nullptr, &readyToPresent[i]);
            if (readyToPresent[i] == nullptr)
                panic("Could not create VkSemaphore: {}\n",
                    vkResultToStr(readyResult));
        }

        for (u32 i = 0; i < vk->frames.size(); ++i)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            [[maybe_unused]]
            VkResult availableResult = vkCreateSemaphore(
                vk->device, &semaphoreInfo, nullptr, &imageAvailable[i]);
            if (imageAvailable[i] == nullptr)
                panic("Could not create VkSemaphore: {}\n",
                    vkResultToStr(availableResult));
        }
    }
    else
    {
        swapchain = nullptr;
    }

    imageIdx = (u32)-1;

    if (oldSwapchain != nullptr)
        vkDestroySwapchainKHR(vk->device, oldSwapchain, nullptr);
}

u32 GpuSwapchainVk::width() const
{
    return imageWidth;
}

u32 GpuSwapchainVk::height() const
{
    return imageHeight;
}

Format GpuSwapchainVk::format() const
{
    return imageFormat;
}

u32 GpuSwapchainVk::imageCount() const
{
    return assertCast<u32>(images.size());
}

bool GpuSwapchainVk::frameAvailable() const
{
    return imageIdx < views.size();
}

GpuImage* GpuSwapchainVk::currentImage() const
{
    return const_cast<GpuImageVk*>(&images[imageIdx]);
}

GpuView* GpuSwapchainVk::currentView() const
{
    return const_cast<GpuViewVk*>(&views[imageIdx]);
}

GpuSwapchainVk::~GpuSwapchainVk() noexcept
{
    for (GpuViewVk& view : views)
    {
        if (view.view != nullptr)
        {
            vkDestroyImageView(vk->device, view.view, nullptr);
            view.view = nullptr;
        }
    }

    for (GpuImageVk& image : images)
        image.image = nullptr;

    for (VkSemaphore semaphore : readyToPresent)
    {
        if (semaphore != nullptr)
            vkDestroySemaphore(vk->device, semaphore, nullptr);
    }
    for (VkSemaphore semaphore : imageAvailable)
    {
        if (semaphore != nullptr)
            vkDestroySemaphore(vk->device, semaphore, nullptr);
    }

    if (swapchain != nullptr)
        vkDestroySwapchainKHR(vk->device, swapchain, nullptr);
    if (surface != nullptr)
        vkDestroySurfaceKHR(vk->instance, surface, nullptr);
}

} // namespace vk

namespace internal {

void initImGuiGpu(
    GpuSwapchain* swap,
    Format colorFormat,
    Format depthFormat,
    Format stencilFormat)
{
    HG_ASSERT(colorFormat != Format::undefined);

    vk::GpuVk* vk = static_cast<vk::GpuSwapchainVk*>(swap)->vk;

    ArenaScope scratch = getScratch();

    VkFormat colorVkFormat = vk::formatToVk(colorFormat);
    VkFormat depthVkFormat = vk::formatToVk(depthFormat);
    VkFormat stencilVkFormat = vk::formatToVk(stencilFormat);

    ImGui_ImplVulkan_InitInfo imguiInfo{};
    imguiInfo.Instance = vk->instance;
    imguiInfo.PhysicalDevice = vk->physicalDevice;
    imguiInfo.Device = vk->device;
    imguiInfo.QueueFamily = vk->queueFamily;
    imguiInfo.Queue = vk->queue;
    imguiInfo.DescriptorPoolSize = 1000;
    imguiInfo.MinImageCount = swap->imageCount();
    imguiInfo.ImageCount = swap->imageCount();
    imguiInfo.MinAllocationSize = 1 << 20;
    imguiInfo.UseDynamicRendering = true;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthVkFormat;
    imguiInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = stencilVkFormat;

    ImGui_ImplVulkan_Init(&imguiInfo);
}

void deinitImGuiGpu()
{
    ImGui_ImplVulkan_Shutdown();
}

void beginImGuiFrameGpu()
{
    ImGui_ImplVulkan_NewFrame();
}

} // namespace internal

void* createImGuiTexture(GpuView* view, GpuLayout layout)
{
    return ImGui_ImplVulkan_AddTexture(static_cast<vk::GpuViewVk*>(view)->sampler, static_cast<vk::GpuViewVk*>(view)->view, vk::gpuLayoutToVk(layout));
}

void destroyImGuiTexture(void* texture)
{
    ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(texture));
}

void renderImGui(GpuCommandBuffer* cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<vk::GpuCommandBufferVk*>(cmd)->cmd);
}

} // namespace hg
