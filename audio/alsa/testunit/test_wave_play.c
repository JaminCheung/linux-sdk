#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <types.h>
#include <utils/log.h>
#include <audio/alsa/wave_player.h>

#define LOG_TAG "test_wave_play"

const char* snd_device = "default";

int main(int argc, char *argv[]) {
    int fd;
    int error = 0;

    if (argc != 2) {
        LOGE("Usage: %s FILE.wav\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open %s\n", argv[1]);
        return -1;
    }

    error = wave_play_file(snd_device, fd);
    if (error < 0) {
        LOGE("Failed to play wave\n");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}
