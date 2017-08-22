#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <types.h>
#include <utils/log.h>
#include <audio/alsa/wave_recorder.h>
#include <audio/alsa/mixer_controller.h>

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
static struct wave_recorder* recorder;
static struct mixer_controller* mixer;

int main(int argc, char *argv[]) {
    int fd;
    int error = 0;

    if (argc != 3) {
        LOGE("Usage: %s FILE.wav\n", argv[0]);
        return -1;
    }

    remove(argv[1]);

    fd = open(argv[1], O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        LOGE("Failed to create %s\n", argv[1]);
        return -1;
    }

    recorder = get_wave_recorder();
    mixer = get_mixer_controller();

    error = mixer->init();
    if (error < 0) {
        LOGE("Failed to init recorder\n");
        goto error;
    }

    int volume = strtol(argv[2], NULL, 10);

    error = mixer->set_volume("Digital", CAPTURE, volume);
    if (error < 0) {
        LOGE("Failed to set volume\n");
        goto error;
    }

    volume = mixer->get_volume("Digital", CAPTURE);
    if (volume < 0) {
        LOGE("Failed to get_volume\n");
        goto error;
    }

    LOGI("Record Volume: %d\n", volume);

    error = recorder->record_file(snd_device, fd, DEFAULT_CHANNELS,
            DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_LENGTH, DEFAULT_DURATION_TIME);
    if (error < 0) {
        LOGE("Failed to record wave file\n");
        goto error;
    }

    close(fd);

    return 0;

error:
    if (fd > 0)
        close(fd);

    return -1;
}
