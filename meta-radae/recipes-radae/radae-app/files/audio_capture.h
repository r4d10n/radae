/*
 * ALSA Audio Capture and Playback Interface
 */

#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <stdint.h>
#include <stddef.h>
#include <alsa/asoundlib.h>

#define AUDIO_PERIOD_SIZE 160   // 10ms @ 16kHz
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_CHANNELS 1

typedef struct {
    snd_pcm_t *capture_handle;
    snd_pcm_t *playback_handle;
    unsigned int sample_rate;
    unsigned int channels;
    snd_pcm_uframes_t period_size;
} audio_interface_t;

// Forward declaration
typedef struct radae_config radae_config_t;

/**
 * Initialize audio interface
 * @param audio Audio interface structure
 * @param config RADAE configuration
 * @return 0 on success, -1 on error
 */
int audio_init(audio_interface_t *audio, radae_config_t *config);

/**
 * Capture audio samples
 * @param audio Audio interface
 * @param buffer Output buffer for samples
 * @param frames Number of frames to capture
 * @return 0 on success, -1 on error
 */
int audio_capture(audio_interface_t *audio, int16_t *buffer, size_t frames);

/**
 * Playback audio samples
 * @param audio Audio interface
 * @param buffer Input buffer with samples
 * @param frames Number of frames to play
 * @return 0 on success, -1 on error
 */
int audio_playback(audio_interface_t *audio, int16_t *buffer, size_t frames);

/**
 * Get playback delay
 * @param audio Audio interface
 * @param delay Output delay in frames
 * @return 0 on success, -1 on error
 */
int audio_get_delay(audio_interface_t *audio, snd_pcm_sframes_t *delay);

/**
 * Cleanup audio interface
 * @param audio Audio interface
 */
void audio_cleanup(audio_interface_t *audio);

#endif /* AUDIO_CAPTURE_H */
