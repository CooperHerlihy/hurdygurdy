#include "hg/audio.hpp"

namespace hg {

template<>
void assetLoadImpl(AssetData<Sound>* data)
{
    static_cast<void>(data);
    HG_PANIC("Load audio file impl : TODO\n");
}

void AudioPlayer::update()
{
    for (u64 i = sounds.count - 1; i < sounds.count; --i)
    {
        if (sounds[i].queuedSize() == 0)
        {
            sounds.removeShift(i);
        }
    }

    for (u32 i = 0; i < music.count; ++i)
    {
        AudioPlayerMusic& m = music[i];
        if (!m.playing)
            continue;

        u32 total = m.sound->frequency / 16;
        u32 queued = m.stream.queuedSize();
        if (queued >= total)
            continue;
        u32 toPush = total - queued;

        ArenaScope scratch = getScratch();
        ArrayTemp<f32> queue{scratch, 0, toPush};

        while (queue.count < toPush)
        {
            if (m.pos == m.sound->data.count)
                m.pos = 0;

            u64 toQueue = std::min(toPush - queue.count, m.sound->data.count - m.pos);
            HG_ASSERT(queue.count + toQueue <= toPush);
            u64 end = queue.count;
            queue.count += toQueue;
            memcpy(&queue[end], &m.sound->data[m.pos], toQueue * sizeof(f32));
            m.pos += toQueue;
        }

        m.stream.push(queue);
    }
}

void AudioPlayer::playMusic(const Asset<Sound>& musicSrc)
{
    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].sound == musicSrc)
        {
            music[i].playing = true;
            return;
        }
    }

    AudioPlayerMusic* track = music.push();
    track->stream = {musicSrc->frequency, musicSrc->channels};
    track->sound = musicSrc.clone();
    track->pos = 0;
    track->playing = true;
}

void AudioPlayer::killMusic(const Asset<Sound>& musicSrc)
{
    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].sound == musicSrc)
        {
            music.removeShift(i);
            return;
        }
    }
}

void AudioPlayer::pauseMusic(const Asset<Sound>& musicSrc)
{
    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].sound == musicSrc)
        {
            music[i].playing = false;
            return;
        }
    }
}

void AudioPlayer::setMusicGain(const Asset<Sound>& musicSrc, f32 gain)
{
    for (u32 i = 0; i < music.count; ++i)
    {
        if (music[i].sound == musicSrc)
        {
            music[i].stream.setGain(gain);
            return;
        }
    }
}

void AudioPlayer::playSound(const Asset<Sound>& sound, f32 gain)
{
    AudioStream* stream = sounds.push({sound->frequency, sound->channels});
    stream->setGain(gain);
    stream->push(sound->data);
}

} // namespace hg
