#include "tests.hpp"
#include "hg/render2d.hpp"
#include "hg/utility.hpp"

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
    tex.format = Format::rgba8_srgb;
    tex.pixels.resize(assertCast<u64>(width) * height * sizeof(u32));
    u32* pixels = reinterpret_cast<u32*>(tex.pixels.data());
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
    ASSERT(textureStorePng(original, path));

    std::optional<TextureData> loaded = textureLoad(path);
    ASSERT(loaded.has_value());
    ASSERT(loaded->width == 4);
    ASSERT(loaded->height == 3);
    ASSERT(loaded->depth == 1);
    ASSERT(loaded->format == Format::rgba8_srgb);

    const u32* originalPixels = reinterpret_cast<const u32*>(original.pixels.data());
    const u32* loadedPixels = reinterpret_cast<const u32*>(loaded->pixels.data());
    for (u32 i = 0; i < 4 * 3; ++i)
        ASSERT(loadedPixels[i] == originalPixels[i]);
}

TEST(testTextureStoreLoadJpg)
{
    ensureTestDir();

    TextureData original = makePattern(8, 8);
    std::string path = makePath("tex.jpg");
    ASSERT(textureStoreJpg(original, 90, path));

    std::optional<TextureData> loaded = textureLoad(path);
    ASSERT(loaded.has_value());
    ASSERT(loaded->width == 8);
    ASSERT(loaded->height == 8);
    ASSERT(loaded->format == Format::rgba8_srgb);
}

TEST(testTextureLoadUndecodable)
{
    ensureTestDir();
    std::string path = makePath("garbage.bin");
    std::vector<byte> garbage{static_cast<byte>('n'), static_cast<byte>('o'), static_cast<byte>('t'),
        static_cast<byte>(' '), static_cast<byte>('a'), static_cast<byte>('n'), static_cast<byte>(' '),
        static_cast<byte>('i'), static_cast<byte>('m'), static_cast<byte>('a'), static_cast<byte>('g'),
        static_cast<byte>('e')};
    ASSERT(binaryStore(garbage, path));

    setError("");
    std::optional<TextureData> loaded = textureLoad(path);
    ASSERT(!loaded.has_value());
    ASSERT(getError().length() > 0);
}

TEST(testTextureLoadMissing)
{
    setError("");
    std::optional<TextureData> loaded = textureLoad(makePath("missing_file"));
    ASSERT(!loaded.has_value());
    ASSERT(getError().length() > 0);
}

TEST(testTextureStoreBadDirectory)
{
    TextureData tex = makePattern(2, 2);

    setError("");
    ASSERT(!textureStorePng(tex, std::string{testDir} + "/hg_nonexistent_dir_12345/tex.png"));
    ASSERT(getError().length() > 0);

    setError("");
    ASSERT(!textureStoreJpg(tex, 90, std::string{testDir} + "/hg_nonexistent_dir_12345/tex.jpg"));
    ASSERT(getError().length() > 0);
}
