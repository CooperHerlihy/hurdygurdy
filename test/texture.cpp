#include "tests.hpp"
#include "hg/render2d.hpp"
#include "hg/error.hpp"

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace hg;

static constexpr const char* testDir = "/tmp/hg_texture_test";

static void ensureTestDir()
{
    std::filesystem::create_directories(testDir);
}

static std::string makePath(const char* name)
{
    return std::string{testDir} + "/" + name;
}

static TextureData makePattern(u32 width, u32 height)
{
    TextureData tex{};
    tex.width = width;
    tex.height = height;
    tex.depth = 1;
    tex.format = Format_rgba8_srgb;
    tex.pixels = std::malloc(static_cast<u64>(width) * height * sizeof(u32));
    u32* pixels = static_cast<u32*>(tex.pixels);
    for (u32 y = 0; y < height; ++y)
    {
        for (u32 x = 0; x < width; ++x)
            pixels[y * width + x] = 0xFF000000u | (x * 16u) | ((y * 16u) << 8u) | ((x * y * 16u) << 16u);
    }
    return tex;
}

TEST(testTextureStoreLoadPng)
{
    ensureTestDir();

    TextureData original = makePattern(4, 3);
    std::string path = makePath("tex.png");
    ASSERT(storeTexturePng(original, path));

    Asset<TextureData> loaded = load<TextureData>(path);
    ASSERT(loaded.data != nullptr);
    ASSERT(loaded->width == 4);
    ASSERT(loaded->height == 3);
    ASSERT(loaded->depth == 1);
    ASSERT(loaded->format == Format_rgba8_srgb);

    const u32* originalPixels = static_cast<const u32*>(original.pixels);
    const u32* loadedPixels = static_cast<const u32*>(loaded->pixels);
    for (u32 i = 0; i < 4 * 3; ++i)
        ASSERT(loadedPixels[i] == originalPixels[i]);
}

TEST(testTextureStoreLoadJpg)
{
    ensureTestDir();

    TextureData original = makePattern(8, 8);
    std::string path = makePath("tex.jpg");
    ASSERT(storeTextureJpg(original, 90, path));

    Asset<TextureData> loaded = load<TextureData>(path);
    ASSERT(loaded.data != nullptr);
    ASSERT(loaded->width == 8);
    ASSERT(loaded->height == 8);
    ASSERT(loaded->format == Format_rgba8_srgb);
}

TEST(testTextureLoadUndecodable)
{
    ensureTestDir();
    std::string path = makePath("garbage.bin");
    std::vector<u8> garbage{'n', 'o', 't', ' ', 'a', 'n', ' ', 'i', 'm', 'a', 'g', 'e'};
    ASSERT(storeFile(garbage, path));

    setError("");
    Asset<TextureData> loaded = load<TextureData>(path);
    ASSERT(loaded->pixels == nullptr);
    ASSERT(loaded->width == 0);
    ASSERT(loaded->height == 0);
    ASSERT(getError().length() > 0);
}

TEST(testTextureLoadMissing)
{
    setError("");
    Asset<TextureData> loaded = load<TextureData>(makePath("missing_file"));
    ASSERT(loaded->pixels == nullptr);
    ASSERT(loaded->width == 0);
    ASSERT(getError().length() > 0);
}

TEST(testTextureStoreBadDirectory)
{
    TextureData tex = makePattern(2, 2);

    setError("");
    ASSERT(!storeTexturePng(tex, std::string{testDir} + "/hg_nonexistent_dir_12345/tex.png"));
    ASSERT(getError().length() > 0);

    setError("");
    ASSERT(!storeTextureJpg(tex, 90, std::string{testDir} + "/hg_nonexistent_dir_12345/tex.jpg"));
    ASSERT(getError().length() > 0);
}
