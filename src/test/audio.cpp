#include "tests.hpp"
#include "hg/audio.hpp"
#include "hg/error.hpp"

#include <filesystem>
#include <string>
#include <vector>

using namespace hg;

static constexpr const char* testDir = "/tmp/hg_audio_test";

static void ensureTestDir()
{
    std::filesystem::create_directories(testDir);
}

static std::string makePath(const char* name)
{
    return std::string{testDir} + "/" + name;
}

TEST(testSoundSerialRoundTrip)
{
    ensureTestDir();

    Sound original{};
    original.frequency = 48000;
    original.channels = 2;
    original.data.resize(6);
    for (u64 i = 0; i < original.data.size(); ++i)
        original.data[i] = static_cast<f32>(i) * 0.25f - 0.5f;

    std::string path = makePath("serial_roundtrip");
    ASSERT(store(original, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a.data != nullptr);
    ASSERT(a->frequency == 48000);
    ASSERT(a->channels == 2);
    ASSERT(a->data.size() == 6);
    for (u64 i = 0; i < a->data.size(); ++i)
        ASSERT(a->data[i] == original.data[i]);
}

TEST(testSoundWavStoreLoadMono)
{
    ensureTestDir();

    Sound original{};
    original.frequency = 22050;
    original.channels = 1;
    original.data.resize(8);
    static constexpr f32 samples[] = {0.0f, 0.25f, -0.5f, 0.75f, -1.0f, 0.5f, -0.25f, 1.0f};
    for (u64 i = 0; i < original.data.size(); ++i)
        original.data[i] = samples[i];

    std::string path = makePath("wav_mono");
    ASSERT(storeSoundWav(original, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 22050);
    ASSERT(a->channels == 1);
    ASSERT(a->data.size() == 8);
    for (u64 i = 0; i < a->data.size(); ++i)
        ASSERT(a->data[i] == samples[i]);
}

TEST(testSoundWavStoreLoadStereo)
{
    ensureTestDir();

    Sound original{};
    original.frequency = 44100;
    original.channels = 2;
    original.data.resize(8);
    static constexpr f32 samples[] = {0.1f, -0.1f, 0.2f, -0.2f, 0.3f, -0.3f, 0.4f, -0.4f};
    for (u64 i = 0; i < original.data.size(); ++i)
        original.data[i] = samples[i];

    std::string path = makePath("wav_stereo");
    ASSERT(storeSoundWav(original, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 44100);
    ASSERT(a->channels == 2);
    ASSERT(a->data.size() == 8);
    for (u64 i = 0; i < a->data.size(); ++i)
        ASSERT(a->data[i] == samples[i]);
}

TEST(testSoundWavStoreNoChannels)
{
    Sound original{};
    original.frequency = 48000;
    original.channels = 0;

    std::string path = makePath("wav_no_channels");
    setError("");
    ASSERT(!storeSoundWav(original, path));
    ASSERT(getError().length() > 0);
}

TEST(testSoundLoadUnknownType)
{
    ensureTestDir();
    std::string path = makePath("unknown_type");
    std::vector<u8> hello{'h', 'e', 'l', 'l', 'o'};
    ASSERT(storeFile(hello, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 0);
    ASSERT(a->channels == 0);
    ASSERT(a->data.size() == 0);
    ASSERT(getError().length() > 0);
}

TEST(testSoundLoadMissing)
{
    std::string path = makePath("missing_file");

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 0);
    ASSERT(a->channels == 0);
    ASSERT(a->data.size() == 0);
    ASSERT(getError().length() > 0);
}

TEST(testSoundLoadBadSerialData)
{
    ensureTestDir();
    std::string path = makePath("bad_serial");

    // Valid serial binary, but with a structure that does not match a Sound
    Serializer w = serialWriter();
    i64 wrong = 5;
    ASSERT(serialize(&w, &wrong));
    std::vector<u8> bin = writeSerialBinary(&w);
    ASSERT(storeFile(bin, path));

    Asset<Sound> a = load<Sound>(path);
    ASSERT(a->frequency == 0);
    ASSERT(a->channels == 0);
    ASSERT(a->data.size() == 0);
    ASSERT(getError().length() > 0);
}
