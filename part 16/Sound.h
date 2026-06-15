#pragma once
#include <SDL3/SDL.h>

// Minimal sound with zero asset files: we synthesise short tones on the fly
// and push them to an SDL3 audio stream. Each game event plays a little blip.
class Sound
{
public:
    bool init();          // open the audio device; safe to call once
    void shutdown();

    // Event blips.
    void hit();
    void pickup();
    void descend();
    void zap();
    void die();
    void win();

private:
    void beep(float freqHz, int ms, float volume);

    SDL_AudioStream* m_stream = nullptr;
    int              m_rate   = 44100;
};
