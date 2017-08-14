#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <types.h>
#include <utils/log.h>
#include <audio/alsa/wave_recorder.h>

#define LOG_TAG "test_wave_record"

#define DEFAULT_CHANNELS         (2)
#define DEFAULT_SAMPLE_RATE      (8000)
#define DEFAULT_SAMPLE_LENGTH    (16)
#define DEFAULT_DURATION_TIME    (10)

/*
 * DMIC
 */
//const char* snd_device = "hw:0,2";

/*
 * AMIC
 */
const char* snd_device = "default";

int main(int argc, char *argv[]) {
    int fd;
    int error = 0;

    if (argc != 2) {
        LOGE("Usage: %s FILE.wav\n", argv[0]);
        return -1;
    }

    remove(argv[1]);

    fd = open(argv[1], O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        LOGE("Failed to create %s\n", argv[1]);
        return -1;
    }

    error = wave_record_file(snd_device, fd, DEFAULT_CHANNELS,
            DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_LENGTH, DEFAULT_DURATION_TIME);
    if (error < 0) {
        LOGE("Failed to record wave file\n");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}
