#include "tests.hpp"
#include "hg/audio.hpp"
#include "hg/utility.hpp"

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
        original.data[i] = assertCast<f32>(i) * 0.25f - 0.5f;

    std::string path = makePath("serial_roundtrip");
    std::vector<byte> bin = serialStoreToBinary(original);
    ASSERT(binaryStore(bin, path));

    std::optional<Sound> a = soundLoad(path);
    ASSERT(a.has_value());
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
    ASSERT(soundStoreWav(original, path));

    std::optional<Sound> a = soundLoad(path);
    ASSERT(a.has_value());
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
    ASSERT(soundStoreWav(original, path));

    std::optional<Sound> a = soundLoad(path);
    ASSERT(a.has_value());
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
    ASSERT(!soundStoreWav(original, path));
    ASSERT(getError().length() > 0);
}

TEST(testSoundLoadUnknownType)
{
    ensureTestDir();
    std::string path = makePath("unknown_type");
    std::vector<byte> hello{static_cast<byte>('h'), static_cast<byte>('e'), static_cast<byte>('l'),
        static_cast<byte>('l'), static_cast<byte>('o')};
    ASSERT(binaryStore(hello, path));

    setError("");
    std::optional<Sound> a = soundLoad(path);
    ASSERT(!a.has_value());
    ASSERT(getError().length() > 0);
}

TEST(testSoundLoadMissing)
{
    std::string path = makePath("missing_file");

    setError("");
    std::optional<Sound> a = soundLoad(path);
    ASSERT(!a.has_value());
    ASSERT(getError().length() > 0);
}

TEST(testSoundLoadBadSerialData)
{
    ensureTestDir();
    std::string path = makePath("bad_serial");

    // Valid serial binary, but with a structure that does not match a Sound
    Serializer w = Serializer::writer();
    i64 wrong = 5;
    ASSERT(serialize(&w, &wrong));
    std::vector<byte> bin = w.writeBinary();
    ASSERT(binaryStore(bin, path));

    setError("");
    std::optional<Sound> a = soundLoad(path);
    ASSERT(!a.has_value());
    ASSERT(getError().length() > 0);
}

TEST(testAudioDeviceDummy)
{
    std::unique_ptr<AudioDeviceDummy> dev = AudioDeviceDummy::create();
    ASSERT(dev != nullptr);
    ASSERT(dev->getBackend() == AudioBackend::none);

    // setCallback/unsetCallback are no-ops and must not crash
    AudioConfig config{};
    dev->setCallback([](std::span<f32>, AudioConfig) {}, config);
    dev->unsetCallback();
}

TEST(testAudioDeviceCreateNoneHint)
{
    AudioDevice::hintBackend(AudioBackend::none);
    std::unique_ptr<AudioDevice> dev = AudioDevice::create();
    ASSERT(dev != nullptr);
    ASSERT(dev->getBackend() == AudioBackend::none);
    // Restore the platform default so other tests are unaffected
#if defined(HG_PLATFORM_LINUX)
    AudioDevice::hintBackend(AudioBackend::pipewire);
#elif defined(HG_PLATFORM_WINDOWS)
    AudioDevice::hintBackend(AudioBackend::wasapi);
#else
    AudioDevice::hintBackend(AudioBackend::sdl3);
#endif
}

// Records callback wiring so AudioPlayer lifetime can be verified
class MockAudioDevice : public AudioDevice {
public:
    AudioCallback callback;
    AudioConfig config{};
    bool hasCallback = false;

    void setCallback(AudioCallback cb, const AudioConfig& preferredConfig = {}) override
    {
        callback = std::move(cb);
        config = preferredConfig;
        hasCallback = true;
    }

    void unsetCallback() override
    {
        hasCallback = false;
        callback = nullptr;
    }

    AudioBackend getBackend() override
    {
        return AudioBackend::none;
    }
};

TEST(testAudioPlayerCallbackWiring)
{
    MockAudioDevice dev;
    ASSERT(!dev.hasCallback);
    {
        AudioPlayer player{&dev};
        ASSERT(dev.hasCallback);
    }
    ASSERT(!dev.hasCallback);
}

TEST(testAudioPlayerSoundPlaysOnce)
{
    MockAudioDevice dev;
    AudioPlayer player{&dev};

    Sound sound{};
    sound.frequency = 48000;
    sound.channels = 1;
    sound.data = {1.0f, 2.0f, 3.0f};

    player.playSound(sound);

    std::vector<f32> buf(4, 0.0f);
    player.update(buf, AudioConfig{});
    ASSERT(buf[0] == 1.0f);
    ASSERT(buf[1] == 2.0f);
    ASSERT(buf[2] == 3.0f);
    ASSERT(buf[3] == 0.0f);

    // The sound is consumed after one pass and is not replayed
    std::vector<f32> buf2(4, 0.0f);
    player.update(buf2, AudioConfig{});
    ASSERT(buf2[0] == 0.0f);
    ASSERT(buf2[1] == 0.0f);
    ASSERT(buf2[2] == 0.0f);
    ASSERT(buf2[3] == 0.0f);
}

TEST(testAudioPlayerSoundGain)
{
    MockAudioDevice dev;
    AudioPlayer player{&dev};

    Sound sound{};
    sound.data = {2.0f, 4.0f};

    player.playSound(sound, 0.5f);

    std::vector<f32> buf(2, 0.0f);
    player.update(buf, AudioConfig{});
    ASSERT(buf[0] == 1.0f);
    ASSERT(buf[1] == 2.0f);
}

TEST(testAudioPlayerMusicLoops)
{
    MockAudioDevice dev;
    AudioPlayer player{&dev};

    Sound music{};
    music.data = {1.0f, 2.0f};

    player.playMusic(music);

    std::vector<f32> buf(4, 0.0f);
    player.update(buf, AudioConfig{});
    ASSERT(buf[0] == 1.0f);
    ASSERT(buf[1] == 2.0f);
    ASSERT(buf[2] == 1.0f);
    ASSERT(buf[3] == 2.0f);
}

TEST(testAudioPlayerPauseResumeMusic)
{
    MockAudioDevice dev;
    AudioPlayer player{&dev};

    Sound music{};
    music.data = {1.0f, 2.0f};

    player.playMusic(music);
    std::vector<f32> buf(2, 0.0f);
    player.update(buf, AudioConfig{});
    ASSERT(buf[0] == 1.0f);
    ASSERT(buf[1] == 2.0f);

    player.pauseMusic(music);
    std::vector<f32> buf2(2, 0.0f);
    player.update(buf2, AudioConfig{});
    ASSERT(buf2[0] == 0.0f);
    ASSERT(buf2[1] == 0.0f);

    // Resuming with playMusic keeps the position and continues playback
    player.playMusic(music);
    std::vector<f32> buf3(2, 0.0f);
    player.update(buf3, AudioConfig{});
    ASSERT(buf3[0] == 1.0f);
    ASSERT(buf3[1] == 2.0f);
}

TEST(testAudioPlayerSetMusicGain)
{
    MockAudioDevice dev;
    AudioPlayer player{&dev};

    Sound music{};
    music.data = {1.0f, 1.0f};

    player.playMusic(music);
    player.setMusicGain(music, 0.25f);

    std::vector<f32> buf(2, 0.0f);
    player.update(buf, AudioConfig{});
    ASSERT(buf[0] == 0.25f);
    ASSERT(buf[1] == 0.25f);
}

TEST(testAudioPlayerKillMusicUnknownNoCrash)
{
    MockAudioDevice dev;
    AudioPlayer player{&dev};

    Sound music{};
    music.data = {1.0f};

    // Killing a track that was never added must be a safe no-op
    player.killMusic(music);

    player.playMusic(music);
    player.killMusic(music);
}

TEST(testAudioPlayerMixedSoundsAndMusic)
{
    MockAudioDevice dev;
    AudioPlayer player{&dev};

    Sound sound{};
    sound.data = {10.0f};
    Sound music{};
    music.data = {1.0f, 1.0f};

    player.playMusic(music);
    player.playSound(sound);

    std::vector<f32> buf(2, 0.0f);
    player.update(buf, AudioConfig{});
    // Sound mixes additively with music
    ASSERT(buf[0] == 11.0f);
    ASSERT(buf[1] == 1.0f);

    // The one-shot sound is gone, only music remains
    std::vector<f32> buf2(2, 0.0f);
    player.update(buf2, AudioConfig{});
    ASSERT(buf2[0] == 1.0f);
    ASSERT(buf2[1] == 1.0f);
}
