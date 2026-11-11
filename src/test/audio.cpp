#include "tests.hpp"
#include "hg/audio.hpp"
#include "hg/filesystem.hpp"

using namespace hg;

static constexpr const char* testDir = "/tmp/hg_audio_test";

static void ensureTestDir()
{
    makeDirectoryRecursive(testDir);
}

static void makePath(FilePath& path, const char* name)
{
    path.appendPath(testDir);
    path.appendPath(name);
}

TEST(testSoundSerialRoundTrip)
{
    ensureTestDir();

    Sound original{};
    original.frequency = 48000;
    original.channels = 2;
    original.data = Array<f32>{6};
    for (u64 i = 0; i < original.data.count; ++i)
        original.data.vals[i] = static_cast<f32>(i) * 0.25f - 0.5f;

    Arena arena{512};
    FilePath path{&arena};
    makePath(path, "serial_roundtrip");
    ASSERT(store(original, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a.data != nullptr);
    ASSERT(a->frequency == 48000);
    ASSERT(a->channels == 2);
    ASSERT(a->data.count == 6);
    for (u64 i = 0; i < a->data.count; ++i)
        ASSERT(a->data.vals[i] == original.data.vals[i]);
}

TEST(testSoundWavStoreLoadMono)
{
    ensureTestDir();

    Sound original{};
    original.frequency = 22050;
    original.channels = 1;
    original.data = Array<f32>{8};
    static constexpr f32 samples[] = {0.0f, 0.25f, -0.5f, 0.75f, -1.0f, 0.5f, -0.25f, 1.0f};
    for (u64 i = 0; i < original.data.count; ++i)
        original.data.vals[i] = samples[i];

    Arena arena{512};
    FilePath path{&arena};
    makePath(path, "wav_mono");
    ASSERT(storeSoundWav(original, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 22050);
    ASSERT(a->channels == 1);
    ASSERT(a->data.count == 8);
    for (u64 i = 0; i < a->data.count; ++i)
        ASSERT(a->data.vals[i] == samples[i]);
}

TEST(testSoundWavStoreLoadStereo)
{
    ensureTestDir();

    Sound original{};
    original.frequency = 44100;
    original.channels = 2;
    original.data = Array<f32>{8};
    static constexpr f32 samples[] = {0.1f, -0.1f, 0.2f, -0.2f, 0.3f, -0.3f, 0.4f, -0.4f};
    for (u64 i = 0; i < original.data.count; ++i)
        original.data.vals[i] = samples[i];

    Arena arena{512};
    FilePath path{&arena};
    makePath(path, "wav_stereo");
    ASSERT(storeSoundWav(original, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 44100);
    ASSERT(a->channels == 2);
    ASSERT(a->data.count == 8);
    for (u64 i = 0; i < a->data.count; ++i)
        ASSERT(a->data.vals[i] == samples[i]);
}

TEST(testSoundWavStoreNoChannels)
{
    Sound original{};
    original.frequency = 48000;
    original.channels = 0;

    Arena arena{512};
    FilePath path{&arena};
    makePath(path, "wav_no_channels");
    ASSERT(!storeSoundWav(original, path));
}

TEST(testSoundLoadUnknownType)
{
    ensureTestDir();
    Arena arena{512};
    FilePath path{&arena};
    makePath(path, "unknown_type");
    ASSERT(storeFile(BinaryView{"hello", 5}, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 0);
    ASSERT(a->channels == 0);
    ASSERT(a->data.count == 0);
}

TEST(testSoundLoadMissing)
{
    Arena arena{512};
    FilePath path{&arena};
    makePath(path, "missing_file");

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 0);
    ASSERT(a->channels == 0);
    ASSERT(a->data.count == 0);
}
