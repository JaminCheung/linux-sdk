#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <types.h>
#include <utils/log.h>
#include <audio/alsa/wave_player.h>
#include <audio/alsa/mixer_controller.h>

#define LOG_TAG "test_wave_play"

const char* snd_device = "default";
static struct wave_player* player;
static struct mixer_controller* mixer;

int main(int argc, char *argv[]) {
    int fd;
    int error = 0;

    if (argc != 3) {
        LOGE("Usage: %s FILE.wav VOLUME\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open %s\n", argv[1]);
        return -1;
    }

    player = get_wave_player();

    error = player->init(snd_device);
    if (error < 0) {
        LOGE("Failed to init player\n");
        goto error;
    }

    mixer = get_mixer_controller();
    error = mixer->init();
    if (error < 0) {
        LOGE("Failed to init mixer\n");
        goto error;
    }

    int volume = strtol(argv[2], NULL, 10);

    error = mixer->set_volume("MERCURY", PLAYBACK, volume);
    if (error < 0) {
        LOGE("Failed to set volume\n");
        goto error;
    }

    volume = mixer->get_volume("MERCURY", PLAYBACK);
    if (volume < 0) {
        LOGE("Failed to get_volume\n");
        goto error;
    }

    LOGI("Playback Volume: %d\n", volume);

    error = player->play_wave(fd);
    if (error < 0) {
        LOGE("Failed to play wave\n");
        goto error;
    }

    player->deinit();
    mixer->deinit();

    close(fd);

    return 0;

error:
    if (fd > 0)
        close(fd);
    return -1;
}
