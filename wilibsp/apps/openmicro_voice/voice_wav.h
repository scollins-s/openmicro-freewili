#ifndef VOICE_WAV_H
#define VOICE_WAV_H

#include <stdint.h>
#include <stdbool.h>

#define VOICE_WAV_RATE_HZ  16000u
#define VOICE_WAV_CHANNELS 1u

/** Write mono s16le WAV to path. Returns true on success. */
bool voice_wav_write(const char *path, const int16_t *pcm, unsigned frames);

#endif
