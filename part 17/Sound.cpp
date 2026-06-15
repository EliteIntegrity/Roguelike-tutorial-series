#include "Sound.h"
#include <vector>
#include <cmath>

bool Sound::init()
{
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format   = SDL_AUDIO_F32;     // 32-bit float samples
    spec.channels = 1;                 // mono is plenty for blips
    spec.freq     = m_rate;

    // A NULL callback means we feed the stream ourselves with PutAudioStreamData.
    m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!m_stream)
    {
        SDL_Log("Sound: no audio device (%s) — running silent.", SDL_GetError());
        return false;                  // game runs fine without sound
    }
    SDL_ResumeAudioStreamDevice(m_stream);
    return true;
}

void Sound::shutdown()
{
    if (m_stream) { SDL_DestroyAudioStream(m_stream); m_stream = nullptr; }
}

// Generate `ms` of a sine tone and hand it to the stream. A short linear fade
// in and out stops the clicks you'd otherwise hear at the tone's edges.
void Sound::beep(float freqHz, int ms, float volume)
{
    if (!m_stream) return;

    int samples = m_rate * ms / 1000;
    if (samples <= 0) return;

    std::vector<float> buf(samples);
    int fade = m_rate / 200;                       // ~5ms fade
    for (int i = 0; i < samples; ++i)
    {
        float t = (float)i / (float)m_rate;
        float s = SDL_sinf(2.0f * 3.14159265f * freqHz * t) * volume;

        float env = 1.0f;
        if (i < fade)               env = (float)i / fade;
        else if (i > samples - fade) env = (float)(samples - i) / fade;

        buf[i] = s * env;
    }
    SDL_PutAudioStreamData(m_stream, buf.data(), (int)(buf.size() * sizeof(float)));
}

void Sound::hit()     { beep(180.0f, 70,  0.25f); }
void Sound::pickup()  { beep(880.0f, 60,  0.20f); }
void Sound::descend() { beep(330.0f, 90,  0.22f); beep(220.0f, 110, 0.22f); }
void Sound::zap()     { beep(1200.0f, 50, 0.18f); beep(700.0f, 60, 0.18f); }
void Sound::die()     { beep(140.0f, 220, 0.30f); }
void Sound::win()     { beep(523.0f, 120, 0.25f); beep(659.0f, 120, 0.25f); beep(784.0f, 200, 0.25f); }
