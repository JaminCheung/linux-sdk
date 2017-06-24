#include <stdlib.h>
#include <string.h>

#include <utils/log.h>
#include <lib/zlib/zlib.h>

#define LOG_TAG "test_zlib"

int main(int argc, char* argv[])
{
    Bytef text[] = "zlib compress and uncompress test\n";
    uLong tlen = 0;
    Bytef* buf = NULL;
    uLong blen;

    tlen = (uLong) strlen((const char*) text) + 1;

    blen = compressBound(tlen);
    if((buf = (Bytef*) malloc(sizeof(char) * blen)) == NULL) {
        LOGE("Failed to allocate memory");
        return -1;
    }
    memset(buf, 0, sizeof(buf));

    if(compress(buf, &blen, text, tlen) != Z_OK) {
        LOGE("Failed to compress");
        return -1;
    }

    if(uncompress(text, &tlen, buf, blen) != Z_OK) {
        LOGE("Failed to uncompress");
        return -1;
    }

    LOGI("%s", text);

    if(buf != NULL) {
        free(buf);
        buf = NULL;
    }

    return 0;
}
