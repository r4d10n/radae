/*
 * ALSA Audio Capture and Playback for RADAE
 *
 * Provides low-latency audio I/O for real-time voice processing
 * Supports both capture and playback with configurable parameters
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

#include "audio_capture.h"
#include "config_parser.h"

int audio_init(audio_interface_t *audio, radae_config_t *config) {
    int err;
    snd_pcm_hw_params_t *hw_params;
    unsigned int sample_rate = config->audio_sample_rate;
    unsigned int channels = config->audio_channels;
    snd_pcm_uframes_t period_size = config->audio_period_size;
    snd_pcm_uframes_t buffer_size = config->audio_buffer_size;

    memset(audio, 0, sizeof(audio_interface_t));

    // Open capture device
    err = snd_pcm_open(&audio->capture_handle, config->audio_capture_device,
                       SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        fprintf(stderr, "Cannot open capture device %s: %s\n",
                config->audio_capture_device, snd_strerror(err));
        return -1;
    }

    // Configure capture device
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(audio->capture_handle, hw_params);

    snd_pcm_hw_params_set_access(audio->capture_handle, hw_params,
                                  SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(audio->capture_handle, hw_params,
                                  SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(audio->capture_handle, hw_params, channels);
    snd_pcm_hw_params_set_rate_near(audio->capture_handle, hw_params,
                                     &sample_rate, 0);

    snd_pcm_hw_params_set_period_size_near(audio->capture_handle, hw_params,
                                            &period_size, 0);
    snd_pcm_hw_params_set_buffer_size_near(audio->capture_handle, hw_params,
                                            &buffer_size);

    err = snd_pcm_hw_params(audio->capture_handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "Cannot set capture parameters: %s\n", snd_strerror(err));
        snd_pcm_close(audio->capture_handle);
        return -1;
    }

    // Set software parameters for low latency
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_sw_params_alloca(&sw_params);
    snd_pcm_sw_params_current(audio->capture_handle, sw_params);
    snd_pcm_sw_params_set_start_threshold(audio->capture_handle, sw_params,
                                           period_size);
    snd_pcm_sw_params_set_avail_min(audio->capture_handle, sw_params, period_size);
    snd_pcm_sw_params(audio->capture_handle, sw_params);

    snd_pcm_prepare(audio->capture_handle);

    // Open playback device
    err = snd_pcm_open(&audio->playback_handle, config->audio_playback_device,
                       SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Cannot open playback device %s: %s\n",
                config->audio_playback_device, snd_strerror(err));
        snd_pcm_close(audio->capture_handle);
        return -1;
    }

    // Configure playback device
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(audio->playback_handle, hw_params);

    snd_pcm_hw_params_set_access(audio->playback_handle, hw_params,
                                  SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(audio->playback_handle, hw_params,
                                  SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(audio->playback_handle, hw_params, channels);

    sample_rate = config->audio_sample_rate;
    snd_pcm_hw_params_set_rate_near(audio->playback_handle, hw_params,
                                     &sample_rate, 0);

    snd_pcm_hw_params_set_period_size_near(audio->playback_handle, hw_params,
                                            &period_size, 0);
    snd_pcm_hw_params_set_buffer_size_near(audio->playback_handle, hw_params,
                                            &buffer_size);

    err = snd_pcm_hw_params(audio->playback_handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "Cannot set playback parameters: %s\n", snd_strerror(err));
        snd_pcm_close(audio->capture_handle);
        snd_pcm_close(audio->playback_handle);
        return -1;
    }

    snd_pcm_sw_params_alloca(&sw_params);
    snd_pcm_sw_params_current(audio->playback_handle, sw_params);
    snd_pcm_sw_params_set_start_threshold(audio->playback_handle, sw_params,
                                           period_size);
    snd_pcm_sw_params_set_avail_min(audio->playback_handle, sw_params, period_size);
    snd_pcm_sw_params(audio->playback_handle, sw_params);

    snd_pcm_prepare(audio->playback_handle);

    audio->sample_rate = config->audio_sample_rate;
    audio->channels = config->audio_channels;
    audio->period_size = period_size;

    printf("Audio initialized: %d Hz, %d channels, period=%lu frames\n",
           audio->sample_rate, audio->channels, period_size);

    return 0;
}

int audio_capture(audio_interface_t *audio, int16_t *buffer, size_t frames) {
    int err = snd_pcm_readi(audio->capture_handle, buffer, frames);

    if (err == -EPIPE) {
        // Overrun occurred
        fprintf(stderr, "Capture overrun occurred\n");
        snd_pcm_prepare(audio->capture_handle);
        return -1;
    } else if (err < 0) {
        fprintf(stderr, "Capture error: %s\n", snd_strerror(err));
        return -1;
    } else if (err != (int)frames) {
        fprintf(stderr, "Short read: expected %zu, got %d frames\n", frames, err);
        return -1;
    }

    return 0;
}

int audio_playback(audio_interface_t *audio, int16_t *buffer, size_t frames) {
    int err = snd_pcm_writei(audio->playback_handle, buffer, frames);

    if (err == -EPIPE) {
        // Underrun occurred
        fprintf(stderr, "Playback underrun occurred\n");
        snd_pcm_prepare(audio->playback_handle);
        return -1;
    } else if (err < 0) {
        fprintf(stderr, "Playback error: %s\n", snd_strerror(err));
        return -1;
    } else if (err != (int)frames) {
        fprintf(stderr, "Short write: expected %zu, got %d frames\n", frames, err);
        return -1;
    }

    return 0;
}

void audio_cleanup(audio_interface_t *audio) {
    if (audio->capture_handle) {
        snd_pcm_drain(audio->capture_handle);
        snd_pcm_close(audio->capture_handle);
        audio->capture_handle = NULL;
    }

    if (audio->playback_handle) {
        snd_pcm_drain(audio->playback_handle);
        snd_pcm_close(audio->playback_handle);
        audio->playback_handle = NULL;
    }
}

int audio_get_delay(audio_interface_t *audio, snd_pcm_sframes_t *delay) {
    return snd_pcm_delay(audio->playback_handle, delay);
}
