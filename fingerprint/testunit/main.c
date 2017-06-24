#include <types.h>
#include <utils/log.h>
#include <fingerprint/fingerprint_manager.h>

#define LOG_TAG "test_fingerprint"

static struct fingerprint_manager* fingerprint_manager;

int main(int argc, char *argv[]) {
    fingerprint_manager = get_fingerprint_manager();

    fingerprint_manager->init();

    return 0;
}
