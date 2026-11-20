#include "hg/gpu.hpp"

#include "hg/utility.hpp"

#include "vulkan/vulkan_gpu.hpp"

#include <cmath>

namespace hg {

u32 formatToSize(Format format)
{
    switch (format)
    {
        case Format::undefined: return 0;
        case Format::r8_unorm: return 1;
        case Format::rg8_unorm: return 2;
        case Format::rgba8_unorm: return 4;
        case Format::bgra8_unorm: return 4;
        case Format::rgba8_srgb: return 4;
        case Format::bgra8_srgb: return 4;
        case Format::r16_unorm: return 2;
        case Format::r16_sfloat: return 2;
        case Format::rg16_unorm: return 4;
        case Format::rg16_sfloat: return 4;
        case Format::rgba16_unorm: return 8;
        case Format::rgba16_sfloat: return 8;
        case Format::r32_uint: return 4;
        case Format::r32_sfloat: return 4;
        case Format::rg32_sfloat: return 8;
        case Format::rgba32_sfloat: return 16;
        case Format::a2b10g10r10_unorm_pack32: return 4;
        case Format::b10g11r11_ufloat_pack32: return 4;
        case Format::d16_unorm: return 2;
        case Format::d32_sfloat: return 4;
        case Format::s8_uint: return 1;
        case Format::d16_unorm_s8_uint: return 3;
        case Format::d24_unorm_s8_uint: return 4;
        case Format::d32_sfloat_s8_uint: return 5;
        case Format::bc1_rgb_unorm_block: return 8;
        case Format::bc3_unorm_block: return 16;
        case Format::bc4_unorm_block: return 16;
        case Format::bc5_unorm_block: return 16;
        case Format::bc7_unorm_block: return 16;
        case Format::astc_4x4_unorm_block: return 16;
        case Format::astc_8x8_unorm_block: return 16;
    }
    return 0;
}

u32 getMaxMipmaps(u32 width, u32 height, u32 depth)
{
    u32 max = width > height ? width : height;
    max = max > depth ? max : depth;
    return max == 0 ? 0 : assertCast<u32>(std::log2(assertCast<f32>(max))) + 1;
}

std::unique_ptr<Gpu> Gpu::create(std::span<std::string_view> extensions)
{
    std::unique_ptr<Gpu> gpu = vk::GpuVk::create(extensions);
    if (gpu != nullptr)
        return gpu;

    setError("Could not create gpu: {}", getError());
    return nullptr;
}

} // namespace hg
