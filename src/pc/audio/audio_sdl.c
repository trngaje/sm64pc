#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <time.h>

#include <alsa/asoundlib.h>
#include <stdio.h>

#include "audio_api.h"

#define PCM_DEVICE "default"
static snd_pcm_t *pcm_handle;
static unsigned long int alsa_buffer_size;

static unsigned long get_time(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static bool audio_sdl_init(void) {
	unsigned int pcm, tmp, dir;
	unsigned int rate, channels;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

	rate 	 = 32000;
	channels = 2;

	/* Open the PCM device in playback mode */
	if (pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE,
					SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",
					PCM_DEVICE, snd_strerror(pcm));
        return false;
    }

	/* Allocate parameters object and fill it with default values*/
	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(pcm_handle, params);

	/* Set parameters */
	if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
					SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
		printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
						SND_PCM_FORMAT_S16_LE) < 0)
		printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels) < 0)
		printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0) < 0)
		printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));

	alsa_buffer_size = 1600 + 528 + 544; // five audio buffers from the game
	if (pcm = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, params, &alsa_buffer_size))
		printf("ERROR: Can't set buffer size. %s\n", snd_strerror(pcm));

	/* Write parameters */
	if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0)
		printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));

	/* Resume information */
	printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));

	printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

	snd_pcm_hw_params_get_channels(params, &tmp);
	printf("channels: %i ", tmp);

	if (tmp == 1)
		printf("(mono)\n");
	else if (tmp == 2)
		printf("(stereo)\n");

	snd_pcm_hw_params_get_rate(params, &tmp, 0);
	printf("rate: %d bps\n", tmp);

	snd_pcm_hw_params_get_buffer_size(params, &alsa_buffer_size);
	printf("buffer size: %lu\n", alsa_buffer_size);

	/* Allocate buffer to hold single period */
	snd_pcm_hw_params_get_period_size(params, &frames, 0);
	printf("frames: %u\n", frames);

	snd_pcm_hw_params_get_period_time(params, &tmp, NULL);
	printf("time: %d\n", tmp);

    return true;
}

static int audio_sdl_buffered(void) {
    if (!pcm_handle) {
        return 0;
    }
    snd_pcm_sframes_t ret = snd_pcm_avail(pcm_handle);
    if (ret < 0) {
        return 0;
    }
    ret = alsa_buffer_size - ret;
    return ret < 0 ? 0 : ret;
}

static int audio_sdl_get_desired_buffered(void) {
    return 1100;
}

static void audio_sdl_play(const uint8_t* buff, size_t len) {
    if (!pcm_handle) {
        audio_sdl_init();
    }
	//unsigned long t1 = get_time();
    int frames = len / 4;
    int pcm;
	if (pcm = snd_pcm_writei(pcm_handle, buff, frames) == -EPIPE) {
		printf("XRUN.\n");
		snd_pcm_prepare(pcm_handle);
        // Add some silence to avoid another XRUN
        char buf[1100 * 4 + len];
        memset(buf, 0, 1100 * 4);
        memcpy(buf + 1100 * 4, buff, len);
		if ((pcm = snd_pcm_writei(pcm_handle, buf, 1100 + frames)) < 0) {
			printf("Failed again %d\n", pcm);
		}
	} else if (pcm < 0) {
		printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
		return;
	}
	//fprintf(stderr, "%u ", get_time() - t1);
}

struct AudioAPI audio_sdl = {
    audio_sdl_init,
    audio_sdl_buffered,
    audio_sdl_get_desired_buffered,
    audio_sdl_play
};

