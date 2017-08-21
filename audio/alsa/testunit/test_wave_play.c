#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <types.h>
#include <utils/log.h>
#include <audio/alsa/wave_player.h>

#define LOG_TAG "test_wave_play"

const char* snd_device = "default";
static struct wave_player* player;

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

    error = player->init();
    if (error < 0) {
        LOGE("Failed to init player\n");
        close(fd);
        return -1;
    }

    int volume = strtol(argv[2], NULL, 10);

    error = player->set_volume("MERCURY", volume);
    if (error < 0) {
        LOGE("Failed to set volume\n");
        close(fd);
        return -1;
    }

    player->mute("Digital Playback mute", 0);

    error = player->play_file(snd_device, fd);
    if (error < 0) {
        LOGE("Failed to play wave\n");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}
