#include "voice_wav.h"
#include "ff.h"
#include "platform/diag.h"
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    char     riff[4];
    uint32_t file_size;
    char     wave[4];
    char     fmt_[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data[4];
    uint32_t data_size;
} wav_hdr_t;
#pragma pack(pop)

bool voice_wav_write(const char *path, const int16_t *pcm, unsigned frames) {
    if (!path || !pcm || frames == 0) return false;

    uint32_t data_bytes = frames * sizeof(int16_t);
    wav_hdr_t hdr;
    memcpy(hdr.riff, "RIFF", 4);
    hdr.file_size = 36u + data_bytes;
    memcpy(hdr.wave, "WAVE", 4);
    memcpy(hdr.fmt_, "fmt ", 4);
    hdr.fmt_size = 16;
    hdr.audio_format = 1; /* PCM */
    hdr.num_channels = VOICE_WAV_CHANNELS;
    hdr.sample_rate = VOICE_WAV_RATE_HZ;
    hdr.byte_rate = VOICE_WAV_RATE_HZ * VOICE_WAV_CHANNELS * 2u;
    hdr.block_align = VOICE_WAV_CHANNELS * 2u;
    hdr.bits_per_sample = 16;
    memcpy(hdr.data, "data", 4);
    hdr.data_size = data_bytes;

    FIL fil;
    FRESULT fr = f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        DIAG("voice_wav: f_open(%s) fr=%d\n", path, (int)fr);
        return false;
    }

    UINT bw = 0;
    fr = f_write(&fil, &hdr, sizeof(hdr), &bw);
    if (fr != FR_OK || bw != sizeof(hdr)) {
        DIAG("voice_wav: hdr write fr=%d bw=%u\n", (int)fr, (unsigned)bw);
        f_close(&fil);
        return false;
    }

    const uint8_t *p = (const uint8_t *)pcm;
    uint32_t left = data_bytes;
    while (left) {
        UINT chunk = left > 4096u ? 4096u : (UINT)left;
        fr = f_write(&fil, p, chunk, &bw);
        if (fr != FR_OK || bw != chunk) {
            DIAG("voice_wav: data write fr=%d bw=%u want=%u\n",
                 (int)fr, (unsigned)bw, (unsigned)chunk);
            f_close(&fil);
            return false;
        }
        p += bw;
        left -= bw;
    }

    f_sync(&fil);
    f_close(&fil);
    DIAG("voice_wav: wrote %s (%u bytes pcm)\n", path, (unsigned)data_bytes);
    return true;
}
