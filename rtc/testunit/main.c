#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <time.h>
#include <libqrcode_api.h>

int main(void)
{
    struct rtc_manager *rtc;
    struct rtc_time time;

    rtc = get_rtc_manager();

    time.tm_sec = 0;
    time.tm_min = 0;
    time.tm_hour = 0;
    time.tm_mday = 1;
    time.tm_mon = 0;
    time.tm_year = 2016-1900;
    time.tm_isdst = 0;

    if(rtc->write(&time) < 0) {
        printf("rtc write error\n");
        return -1;
    }

    if(rtc->read(&time) < 0) {
        printf("rtc read error\n");
        return -1;
    }

    printf("rtc time : %d/%d/%d  %d:%d:%d    year_day:%d/365 week_day:%d/6\n",
            time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
            time.tm_hour, time.tm_min, time.tm_sec,
            time.tm_yday, time.tm_wday);

    return 0;
}
