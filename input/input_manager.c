/*
 *  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic Linux plarform SDK project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define _GNU_SOURCE /* for asprintf */

#include <linux/input.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>

#include <utils/log.h>
#include <utils/list.h>
#include <utils/assert.h>
#include <input/input_manager.h>

#define LOG_TAG "input_manager"

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

#ifndef EV_SYN
#define EV_SYN 0
#endif
#ifndef SYN_MAX
#define SYN_MAX 3
#define SYN_CNT (SYN_MAX + 1)
#endif
#ifndef SYN_MT_REPORT
#define SYN_MT_REPORT 2
#endif
#ifndef SYN_DROPPED
#define SYN_DROPPED 3
#endif

#define NAME_ELEMENT(element) [element] = #element

struct listener {
    input_event_listener_t cb;
    struct list_head head;
};

struct input_device {
    char name[NAME_MAX];
    char dev_path[PATH_MAX];
    int fd;
    struct list_head head;
};

LIST_HEAD(input_dev_list);
LIST_HEAD(listener_list);

static int max_fd;
static int inited;
static int started;
static pthread_mutex_t device_list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t listener_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;

static int local_pipe[2];
static const char * const events[EV_MAX + 1] = {
    [0 ... EV_MAX] = NULL,
    NAME_ELEMENT(EV_SYN),           NAME_ELEMENT(EV_KEY),
    NAME_ELEMENT(EV_REL),           NAME_ELEMENT(EV_ABS),
    NAME_ELEMENT(EV_MSC),           NAME_ELEMENT(EV_LED),
    NAME_ELEMENT(EV_SND),           NAME_ELEMENT(EV_REP),
    NAME_ELEMENT(EV_FF),            NAME_ELEMENT(EV_PWR),
    NAME_ELEMENT(EV_FF_STATUS),     NAME_ELEMENT(EV_SW),
};

static const int maxval[EV_MAX + 1] = {
    [0 ... EV_MAX] = -1,
    [EV_SYN] = SYN_MAX,
    [EV_KEY] = KEY_MAX,
    [EV_REL] = REL_MAX,
    [EV_ABS] = ABS_MAX,
    [EV_MSC] = MSC_MAX,
    [EV_SW] = SW_MAX,
    [EV_LED] = LED_MAX,
    [EV_SND] = SND_MAX,
    [EV_REP] = REP_MAX,
    [EV_FF] = FF_MAX,
    [EV_FF_STATUS] = FF_STATUS_MAX,
};


#ifdef INPUT_PROP_SEMI_MT
static const char * const props[INPUT_PROP_MAX + 1] = {
    [0 ... INPUT_PROP_MAX] = NULL,
    NAME_ELEMENT(INPUT_PROP_POINTER),
    NAME_ELEMENT(INPUT_PROP_DIRECT),
    NAME_ELEMENT(INPUT_PROP_BUTTONPAD),
    NAME_ELEMENT(INPUT_PROP_SEMI_MT),
#ifdef INPUT_PROP_TOPBUTTONPAD
    NAME_ELEMENT(INPUT_PROP_TOPBUTTONPAD),
#endif
#ifdef INPUT_PROP_POINTING_STICK
    NAME_ELEMENT(INPUT_PROP_POINTING_STICK),
#endif
};
#endif

static const char * const keys[KEY_MAX + 1] = {
    [0 ... KEY_MAX] = NULL,
    NAME_ELEMENT(KEY_RESERVED),     NAME_ELEMENT(KEY_ESC),
    NAME_ELEMENT(KEY_1),            NAME_ELEMENT(KEY_2),
    NAME_ELEMENT(KEY_3),            NAME_ELEMENT(KEY_4),
    NAME_ELEMENT(KEY_5),            NAME_ELEMENT(KEY_6),
    NAME_ELEMENT(KEY_7),            NAME_ELEMENT(KEY_8),
    NAME_ELEMENT(KEY_9),            NAME_ELEMENT(KEY_0),
    NAME_ELEMENT(KEY_MINUS),        NAME_ELEMENT(KEY_EQUAL),
    NAME_ELEMENT(KEY_BACKSPACE),        NAME_ELEMENT(KEY_TAB),
    NAME_ELEMENT(KEY_Q),            NAME_ELEMENT(KEY_W),
    NAME_ELEMENT(KEY_E),            NAME_ELEMENT(KEY_R),
    NAME_ELEMENT(KEY_T),            NAME_ELEMENT(KEY_Y),
    NAME_ELEMENT(KEY_U),            NAME_ELEMENT(KEY_I),
    NAME_ELEMENT(KEY_O),            NAME_ELEMENT(KEY_P),
    NAME_ELEMENT(KEY_LEFTBRACE),        NAME_ELEMENT(KEY_RIGHTBRACE),
    NAME_ELEMENT(KEY_ENTER),        NAME_ELEMENT(KEY_LEFTCTRL),
    NAME_ELEMENT(KEY_A),            NAME_ELEMENT(KEY_S),
    NAME_ELEMENT(KEY_D),            NAME_ELEMENT(KEY_F),
    NAME_ELEMENT(KEY_G),            NAME_ELEMENT(KEY_H),
    NAME_ELEMENT(KEY_J),            NAME_ELEMENT(KEY_K),
    NAME_ELEMENT(KEY_L),            NAME_ELEMENT(KEY_SEMICOLON),
    NAME_ELEMENT(KEY_APOSTROPHE),       NAME_ELEMENT(KEY_GRAVE),
    NAME_ELEMENT(KEY_LEFTSHIFT),        NAME_ELEMENT(KEY_BACKSLASH),
    NAME_ELEMENT(KEY_Z),            NAME_ELEMENT(KEY_X),
    NAME_ELEMENT(KEY_C),            NAME_ELEMENT(KEY_V),
    NAME_ELEMENT(KEY_B),            NAME_ELEMENT(KEY_N),
    NAME_ELEMENT(KEY_M),            NAME_ELEMENT(KEY_COMMA),
    NAME_ELEMENT(KEY_DOT),          NAME_ELEMENT(KEY_SLASH),
    NAME_ELEMENT(KEY_RIGHTSHIFT),       NAME_ELEMENT(KEY_KPASTERISK),
    NAME_ELEMENT(KEY_LEFTALT),      NAME_ELEMENT(KEY_SPACE),
    NAME_ELEMENT(KEY_CAPSLOCK),     NAME_ELEMENT(KEY_F1),
    NAME_ELEMENT(KEY_F2),           NAME_ELEMENT(KEY_F3),
    NAME_ELEMENT(KEY_F4),           NAME_ELEMENT(KEY_F5),
    NAME_ELEMENT(KEY_F6),           NAME_ELEMENT(KEY_F7),
    NAME_ELEMENT(KEY_F8),           NAME_ELEMENT(KEY_F9),
    NAME_ELEMENT(KEY_F10),          NAME_ELEMENT(KEY_NUMLOCK),
    NAME_ELEMENT(KEY_SCROLLLOCK),       NAME_ELEMENT(KEY_KP7),
    NAME_ELEMENT(KEY_KP8),          NAME_ELEMENT(KEY_KP9),
    NAME_ELEMENT(KEY_KPMINUS),      NAME_ELEMENT(KEY_KP4),
    NAME_ELEMENT(KEY_KP5),          NAME_ELEMENT(KEY_KP6),
    NAME_ELEMENT(KEY_KPPLUS),       NAME_ELEMENT(KEY_KP1),
    NAME_ELEMENT(KEY_KP2),          NAME_ELEMENT(KEY_KP3),
    NAME_ELEMENT(KEY_KP0),          NAME_ELEMENT(KEY_KPDOT),
    NAME_ELEMENT(KEY_ZENKAKUHANKAKU),   NAME_ELEMENT(KEY_102ND),
    NAME_ELEMENT(KEY_F11),          NAME_ELEMENT(KEY_F12),
    NAME_ELEMENT(KEY_RO),           NAME_ELEMENT(KEY_KATAKANA),
    NAME_ELEMENT(KEY_HIRAGANA),     NAME_ELEMENT(KEY_HENKAN),
    NAME_ELEMENT(KEY_KATAKANAHIRAGANA), NAME_ELEMENT(KEY_MUHENKAN),
    NAME_ELEMENT(KEY_KPJPCOMMA),        NAME_ELEMENT(KEY_KPENTER),
    NAME_ELEMENT(KEY_RIGHTCTRL),        NAME_ELEMENT(KEY_KPSLASH),
    NAME_ELEMENT(KEY_SYSRQ),        NAME_ELEMENT(KEY_RIGHTALT),
    NAME_ELEMENT(KEY_LINEFEED),     NAME_ELEMENT(KEY_HOME),
    NAME_ELEMENT(KEY_UP),           NAME_ELEMENT(KEY_PAGEUP),
    NAME_ELEMENT(KEY_LEFT),         NAME_ELEMENT(KEY_RIGHT),
    NAME_ELEMENT(KEY_END),          NAME_ELEMENT(KEY_DOWN),
    NAME_ELEMENT(KEY_PAGEDOWN),     NAME_ELEMENT(KEY_INSERT),
    NAME_ELEMENT(KEY_DELETE),       NAME_ELEMENT(KEY_MACRO),
    NAME_ELEMENT(KEY_MUTE),         NAME_ELEMENT(KEY_VOLUMEDOWN),
    NAME_ELEMENT(KEY_VOLUMEUP),     NAME_ELEMENT(KEY_POWER),
    NAME_ELEMENT(KEY_KPEQUAL),      NAME_ELEMENT(KEY_KPPLUSMINUS),
    NAME_ELEMENT(KEY_PAUSE),        NAME_ELEMENT(KEY_KPCOMMA),
    NAME_ELEMENT(KEY_HANGUEL),      NAME_ELEMENT(KEY_HANJA),
    NAME_ELEMENT(KEY_YEN),          NAME_ELEMENT(KEY_LEFTMETA),
    NAME_ELEMENT(KEY_RIGHTMETA),        NAME_ELEMENT(KEY_COMPOSE),
    NAME_ELEMENT(KEY_STOP),         NAME_ELEMENT(KEY_AGAIN),
    NAME_ELEMENT(KEY_PROPS),        NAME_ELEMENT(KEY_UNDO),
    NAME_ELEMENT(KEY_FRONT),        NAME_ELEMENT(KEY_COPY),
    NAME_ELEMENT(KEY_OPEN),         NAME_ELEMENT(KEY_PASTE),
    NAME_ELEMENT(KEY_FIND),         NAME_ELEMENT(KEY_CUT),
    NAME_ELEMENT(KEY_HELP),         NAME_ELEMENT(KEY_MENU),
    NAME_ELEMENT(KEY_CALC),         NAME_ELEMENT(KEY_SETUP),
    NAME_ELEMENT(KEY_SLEEP),        NAME_ELEMENT(KEY_WAKEUP),
    NAME_ELEMENT(KEY_FILE),         NAME_ELEMENT(KEY_SENDFILE),
    NAME_ELEMENT(KEY_DELETEFILE),       NAME_ELEMENT(KEY_XFER),
    NAME_ELEMENT(KEY_PROG1),        NAME_ELEMENT(KEY_PROG2),
    NAME_ELEMENT(KEY_WWW),          NAME_ELEMENT(KEY_MSDOS),
    NAME_ELEMENT(KEY_COFFEE),       NAME_ELEMENT(KEY_DIRECTION),
    NAME_ELEMENT(KEY_CYCLEWINDOWS),     NAME_ELEMENT(KEY_MAIL),
    NAME_ELEMENT(KEY_BOOKMARKS),        NAME_ELEMENT(KEY_COMPUTER),
    NAME_ELEMENT(KEY_BACK),         NAME_ELEMENT(KEY_FORWARD),
    NAME_ELEMENT(KEY_CLOSECD),      NAME_ELEMENT(KEY_EJECTCD),
    NAME_ELEMENT(KEY_EJECTCLOSECD),     NAME_ELEMENT(KEY_NEXTSONG),
    NAME_ELEMENT(KEY_PLAYPAUSE),        NAME_ELEMENT(KEY_PREVIOUSSONG),
    NAME_ELEMENT(KEY_STOPCD),       NAME_ELEMENT(KEY_RECORD),
    NAME_ELEMENT(KEY_REWIND),       NAME_ELEMENT(KEY_PHONE),
    NAME_ELEMENT(KEY_ISO),          NAME_ELEMENT(KEY_CONFIG),
    NAME_ELEMENT(KEY_HOMEPAGE),     NAME_ELEMENT(KEY_REFRESH),
    NAME_ELEMENT(KEY_EXIT),         NAME_ELEMENT(KEY_MOVE),
    NAME_ELEMENT(KEY_EDIT),         NAME_ELEMENT(KEY_SCROLLUP),
    NAME_ELEMENT(KEY_SCROLLDOWN),       NAME_ELEMENT(KEY_KPLEFTPAREN),
    NAME_ELEMENT(KEY_KPRIGHTPAREN),     NAME_ELEMENT(KEY_F13),
    NAME_ELEMENT(KEY_F14),          NAME_ELEMENT(KEY_F15),
    NAME_ELEMENT(KEY_F16),          NAME_ELEMENT(KEY_F17),
    NAME_ELEMENT(KEY_F18),          NAME_ELEMENT(KEY_F19),
    NAME_ELEMENT(KEY_F20),          NAME_ELEMENT(KEY_F21),
    NAME_ELEMENT(KEY_F22),          NAME_ELEMENT(KEY_F23),
    NAME_ELEMENT(KEY_F24),          NAME_ELEMENT(KEY_PLAYCD),
    NAME_ELEMENT(KEY_PAUSECD),      NAME_ELEMENT(KEY_PROG3),
    NAME_ELEMENT(KEY_PROG4),        NAME_ELEMENT(KEY_SUSPEND),
    NAME_ELEMENT(KEY_CLOSE),        NAME_ELEMENT(KEY_PLAY),
    NAME_ELEMENT(KEY_FASTFORWARD),      NAME_ELEMENT(KEY_BASSBOOST),
    NAME_ELEMENT(KEY_PRINT),        NAME_ELEMENT(KEY_HP),
    NAME_ELEMENT(KEY_CAMERA),       NAME_ELEMENT(KEY_SOUND),
    NAME_ELEMENT(KEY_QUESTION),     NAME_ELEMENT(KEY_EMAIL),
    NAME_ELEMENT(KEY_CHAT),         NAME_ELEMENT(KEY_SEARCH),
    NAME_ELEMENT(KEY_CONNECT),      NAME_ELEMENT(KEY_FINANCE),
    NAME_ELEMENT(KEY_SPORT),        NAME_ELEMENT(KEY_SHOP),
    NAME_ELEMENT(KEY_ALTERASE),     NAME_ELEMENT(KEY_CANCEL),
    NAME_ELEMENT(KEY_BRIGHTNESSDOWN),   NAME_ELEMENT(KEY_BRIGHTNESSUP),
    NAME_ELEMENT(KEY_MEDIA),        NAME_ELEMENT(KEY_UNKNOWN),
    NAME_ELEMENT(KEY_OK),
    NAME_ELEMENT(KEY_SELECT),       NAME_ELEMENT(KEY_GOTO),
    NAME_ELEMENT(KEY_CLEAR),        NAME_ELEMENT(KEY_POWER2),
    NAME_ELEMENT(KEY_OPTION),       NAME_ELEMENT(KEY_INFO),
    NAME_ELEMENT(KEY_TIME),         NAME_ELEMENT(KEY_VENDOR),
    NAME_ELEMENT(KEY_ARCHIVE),      NAME_ELEMENT(KEY_PROGRAM),
    NAME_ELEMENT(KEY_CHANNEL),      NAME_ELEMENT(KEY_FAVORITES),
    NAME_ELEMENT(KEY_EPG),          NAME_ELEMENT(KEY_PVR),
    NAME_ELEMENT(KEY_MHP),          NAME_ELEMENT(KEY_LANGUAGE),
    NAME_ELEMENT(KEY_TITLE),        NAME_ELEMENT(KEY_SUBTITLE),
    NAME_ELEMENT(KEY_ANGLE),        NAME_ELEMENT(KEY_ZOOM),
    NAME_ELEMENT(KEY_MODE),         NAME_ELEMENT(KEY_KEYBOARD),
    NAME_ELEMENT(KEY_SCREEN),       NAME_ELEMENT(KEY_PC),
    NAME_ELEMENT(KEY_TV),           NAME_ELEMENT(KEY_TV2),
    NAME_ELEMENT(KEY_VCR),          NAME_ELEMENT(KEY_VCR2),
    NAME_ELEMENT(KEY_SAT),          NAME_ELEMENT(KEY_SAT2),
    NAME_ELEMENT(KEY_CD),           NAME_ELEMENT(KEY_TAPE),
    NAME_ELEMENT(KEY_RADIO),        NAME_ELEMENT(KEY_TUNER),
    NAME_ELEMENT(KEY_PLAYER),       NAME_ELEMENT(KEY_TEXT),
    NAME_ELEMENT(KEY_DVD),          NAME_ELEMENT(KEY_AUX),
    NAME_ELEMENT(KEY_MP3),          NAME_ELEMENT(KEY_AUDIO),
    NAME_ELEMENT(KEY_VIDEO),        NAME_ELEMENT(KEY_DIRECTORY),
    NAME_ELEMENT(KEY_LIST),         NAME_ELEMENT(KEY_MEMO),
    NAME_ELEMENT(KEY_CALENDAR),     NAME_ELEMENT(KEY_RED),
    NAME_ELEMENT(KEY_GREEN),        NAME_ELEMENT(KEY_YELLOW),
    NAME_ELEMENT(KEY_BLUE),         NAME_ELEMENT(KEY_CHANNELUP),
    NAME_ELEMENT(KEY_CHANNELDOWN),      NAME_ELEMENT(KEY_FIRST),
    NAME_ELEMENT(KEY_LAST),         NAME_ELEMENT(KEY_AB),
    NAME_ELEMENT(KEY_NEXT),         NAME_ELEMENT(KEY_RESTART),
    NAME_ELEMENT(KEY_SLOW),         NAME_ELEMENT(KEY_SHUFFLE),
    NAME_ELEMENT(KEY_BREAK),        NAME_ELEMENT(KEY_PREVIOUS),
    NAME_ELEMENT(KEY_DIGITS),       NAME_ELEMENT(KEY_TEEN),
    NAME_ELEMENT(KEY_TWEN),         NAME_ELEMENT(KEY_DEL_EOL),
    NAME_ELEMENT(KEY_DEL_EOS),      NAME_ELEMENT(KEY_INS_LINE),
    NAME_ELEMENT(KEY_DEL_LINE),
    NAME_ELEMENT(KEY_VIDEOPHONE),       NAME_ELEMENT(KEY_GAMES),
    NAME_ELEMENT(KEY_ZOOMIN),       NAME_ELEMENT(KEY_ZOOMOUT),
    NAME_ELEMENT(KEY_ZOOMRESET),        NAME_ELEMENT(KEY_WORDPROCESSOR),
    NAME_ELEMENT(KEY_EDITOR),       NAME_ELEMENT(KEY_SPREADSHEET),
    NAME_ELEMENT(KEY_GRAPHICSEDITOR),   NAME_ELEMENT(KEY_PRESENTATION),
    NAME_ELEMENT(KEY_DATABASE),     NAME_ELEMENT(KEY_NEWS),
    NAME_ELEMENT(KEY_VOICEMAIL),        NAME_ELEMENT(KEY_ADDRESSBOOK),
    NAME_ELEMENT(KEY_MESSENGER),        NAME_ELEMENT(KEY_DISPLAYTOGGLE),
#ifdef KEY_SPELLCHECK
    NAME_ELEMENT(KEY_SPELLCHECK),
#endif
#ifdef KEY_LOGOFF
    NAME_ELEMENT(KEY_LOGOFF),
#endif
#ifdef KEY_DOLLAR
    NAME_ELEMENT(KEY_DOLLAR),
#endif
#ifdef KEY_EURO
    NAME_ELEMENT(KEY_EURO),
#endif
#ifdef KEY_FRAMEBACK
    NAME_ELEMENT(KEY_FRAMEBACK),
#endif
#ifdef KEY_FRAMEFORWARD
    NAME_ELEMENT(KEY_FRAMEFORWARD),
#endif
#ifdef KEY_CONTEXT_MENU
    NAME_ELEMENT(KEY_CONTEXT_MENU),
#endif
#ifdef KEY_MEDIA_REPEAT
    NAME_ELEMENT(KEY_MEDIA_REPEAT),
#endif
#ifdef KEY_10CHANNELSUP
    NAME_ELEMENT(KEY_10CHANNELSUP),
#endif
#ifdef KEY_10CHANNELSDOWN
    NAME_ELEMENT(KEY_10CHANNELSDOWN),
#endif
#ifdef KEY_IMAGES
    NAME_ELEMENT(KEY_IMAGES),
#endif
    NAME_ELEMENT(KEY_DEL_EOL),      NAME_ELEMENT(KEY_DEL_EOS),
    NAME_ELEMENT(KEY_INS_LINE),     NAME_ELEMENT(KEY_DEL_LINE),
    NAME_ELEMENT(KEY_FN),           NAME_ELEMENT(KEY_FN_ESC),
    NAME_ELEMENT(KEY_FN_F1),        NAME_ELEMENT(KEY_FN_F2),
    NAME_ELEMENT(KEY_FN_F3),        NAME_ELEMENT(KEY_FN_F4),
    NAME_ELEMENT(KEY_FN_F5),        NAME_ELEMENT(KEY_FN_F6),
    NAME_ELEMENT(KEY_FN_F7),        NAME_ELEMENT(KEY_FN_F8),
    NAME_ELEMENT(KEY_FN_F9),        NAME_ELEMENT(KEY_FN_F10),
    NAME_ELEMENT(KEY_FN_F11),       NAME_ELEMENT(KEY_FN_F12),
    NAME_ELEMENT(KEY_FN_1),         NAME_ELEMENT(KEY_FN_2),
    NAME_ELEMENT(KEY_FN_D),         NAME_ELEMENT(KEY_FN_E),
    NAME_ELEMENT(KEY_FN_F),         NAME_ELEMENT(KEY_FN_S),
    NAME_ELEMENT(KEY_FN_B),
    NAME_ELEMENT(KEY_BRL_DOT1),     NAME_ELEMENT(KEY_BRL_DOT2),
    NAME_ELEMENT(KEY_BRL_DOT3),     NAME_ELEMENT(KEY_BRL_DOT4),
    NAME_ELEMENT(KEY_BRL_DOT5),     NAME_ELEMENT(KEY_BRL_DOT6),
    NAME_ELEMENT(KEY_BRL_DOT7),     NAME_ELEMENT(KEY_BRL_DOT8),
    NAME_ELEMENT(KEY_BRL_DOT9),     NAME_ELEMENT(KEY_BRL_DOT10),
#ifdef KEY_NUMERIC_0
    NAME_ELEMENT(KEY_NUMERIC_0),        NAME_ELEMENT(KEY_NUMERIC_1),
    NAME_ELEMENT(KEY_NUMERIC_2),        NAME_ELEMENT(KEY_NUMERIC_3),
    NAME_ELEMENT(KEY_NUMERIC_4),        NAME_ELEMENT(KEY_NUMERIC_5),
    NAME_ELEMENT(KEY_NUMERIC_6),        NAME_ELEMENT(KEY_NUMERIC_7),
    NAME_ELEMENT(KEY_NUMERIC_8),        NAME_ELEMENT(KEY_NUMERIC_9),
    NAME_ELEMENT(KEY_NUMERIC_STAR),     NAME_ELEMENT(KEY_NUMERIC_POUND),
#endif
    NAME_ELEMENT(KEY_BATTERY),
    NAME_ELEMENT(KEY_BLUETOOTH),        NAME_ELEMENT(KEY_BRIGHTNESS_CYCLE),
    NAME_ELEMENT(KEY_BRIGHTNESS_ZERO),
#ifdef KEY_DASHBOARD
    NAME_ELEMENT(KEY_DASHBOARD),
#endif
    NAME_ELEMENT(KEY_DISPLAY_OFF),      NAME_ELEMENT(KEY_DOCUMENTS),
    NAME_ELEMENT(KEY_FORWARDMAIL),      NAME_ELEMENT(KEY_NEW),
    NAME_ELEMENT(KEY_KBDILLUMDOWN),     NAME_ELEMENT(KEY_KBDILLUMUP),
    NAME_ELEMENT(KEY_KBDILLUMTOGGLE),   NAME_ELEMENT(KEY_REDO),
    NAME_ELEMENT(KEY_REPLY),        NAME_ELEMENT(KEY_SAVE),
#ifdef KEY_SCALE
    NAME_ELEMENT(KEY_SCALE),
#endif
    NAME_ELEMENT(KEY_SEND),
    NAME_ELEMENT(KEY_SCREENLOCK),       NAME_ELEMENT(KEY_SWITCHVIDEOMODE),
#ifdef KEY_UWB
    NAME_ELEMENT(KEY_UWB),
#endif
#ifdef KEY_VIDEO_NEXT
    NAME_ELEMENT(KEY_VIDEO_NEXT),
#endif
#ifdef KEY_VIDEO_PREV
    NAME_ELEMENT(KEY_VIDEO_PREV),
#endif
#ifdef KEY_WIMAX
    NAME_ELEMENT(KEY_WIMAX),
#endif
#ifdef KEY_WLAN
    NAME_ELEMENT(KEY_WLAN),
#endif
#ifdef KEY_RFKILL
    NAME_ELEMENT(KEY_RFKILL),
#endif
#ifdef KEY_MICMUTE
    NAME_ELEMENT(KEY_MICMUTE),
#endif
#ifdef KEY_CAMERA_FOCUS
    NAME_ELEMENT(KEY_CAMERA_FOCUS),
#endif
#ifdef KEY_WPS_BUTTON
    NAME_ELEMENT(KEY_WPS_BUTTON),
#endif
#ifdef KEY_TOUCHPAD_TOGGLE
    NAME_ELEMENT(KEY_TOUCHPAD_TOGGLE),
    NAME_ELEMENT(KEY_TOUCHPAD_ON),
    NAME_ELEMENT(KEY_TOUCHPAD_OFF),
#endif
#ifdef KEY_CAMERA_ZOOMIN
    NAME_ELEMENT(KEY_CAMERA_ZOOMIN),    NAME_ELEMENT(KEY_CAMERA_ZOOMOUT),
    NAME_ELEMENT(KEY_CAMERA_UP),        NAME_ELEMENT(KEY_CAMERA_DOWN),
    NAME_ELEMENT(KEY_CAMERA_LEFT),      NAME_ELEMENT(KEY_CAMERA_RIGHT),
#endif
#ifdef KEY_ATTENDANT_ON
    NAME_ELEMENT(KEY_ATTENDANT_ON),     NAME_ELEMENT(KEY_ATTENDANT_OFF),
    NAME_ELEMENT(KEY_ATTENDANT_TOGGLE), NAME_ELEMENT(KEY_LIGHTS_TOGGLE),
#endif

    NAME_ELEMENT(BTN_0),            NAME_ELEMENT(BTN_1),
    NAME_ELEMENT(BTN_2),            NAME_ELEMENT(BTN_3),
    NAME_ELEMENT(BTN_4),            NAME_ELEMENT(BTN_5),
    NAME_ELEMENT(BTN_6),            NAME_ELEMENT(BTN_7),
    NAME_ELEMENT(BTN_8),            NAME_ELEMENT(BTN_9),
    NAME_ELEMENT(BTN_LEFT),         NAME_ELEMENT(BTN_RIGHT),
    NAME_ELEMENT(BTN_MIDDLE),       NAME_ELEMENT(BTN_SIDE),
    NAME_ELEMENT(BTN_EXTRA),        NAME_ELEMENT(BTN_FORWARD),
    NAME_ELEMENT(BTN_BACK),         NAME_ELEMENT(BTN_TASK),
    NAME_ELEMENT(BTN_TRIGGER),      NAME_ELEMENT(BTN_THUMB),
    NAME_ELEMENT(BTN_THUMB2),       NAME_ELEMENT(BTN_TOP),
    NAME_ELEMENT(BTN_TOP2),         NAME_ELEMENT(BTN_PINKIE),
    NAME_ELEMENT(BTN_BASE),         NAME_ELEMENT(BTN_BASE2),
    NAME_ELEMENT(BTN_BASE3),        NAME_ELEMENT(BTN_BASE4),
    NAME_ELEMENT(BTN_BASE5),        NAME_ELEMENT(BTN_BASE6),
    NAME_ELEMENT(BTN_DEAD),         NAME_ELEMENT(BTN_C),
#ifdef BTN_SOUTH
    NAME_ELEMENT(BTN_SOUTH),        NAME_ELEMENT(BTN_EAST),
    NAME_ELEMENT(BTN_NORTH),        NAME_ELEMENT(BTN_WEST),
#else
    NAME_ELEMENT(BTN_A),            NAME_ELEMENT(BTN_B),
    NAME_ELEMENT(BTN_X),            NAME_ELEMENT(BTN_Y),
#endif
    NAME_ELEMENT(BTN_Z),            NAME_ELEMENT(BTN_TL),
    NAME_ELEMENT(BTN_TR),           NAME_ELEMENT(BTN_TL2),
    NAME_ELEMENT(BTN_TR2),          NAME_ELEMENT(BTN_SELECT),
    NAME_ELEMENT(BTN_START),        NAME_ELEMENT(BTN_MODE),
    NAME_ELEMENT(BTN_THUMBL),       NAME_ELEMENT(BTN_THUMBR),
    NAME_ELEMENT(BTN_TOOL_PEN),     NAME_ELEMENT(BTN_TOOL_RUBBER),
    NAME_ELEMENT(BTN_TOOL_BRUSH),       NAME_ELEMENT(BTN_TOOL_PENCIL),
    NAME_ELEMENT(BTN_TOOL_AIRBRUSH),    NAME_ELEMENT(BTN_TOOL_FINGER),
    NAME_ELEMENT(BTN_TOOL_MOUSE),       NAME_ELEMENT(BTN_TOOL_LENS),
    NAME_ELEMENT(BTN_TOUCH),        NAME_ELEMENT(BTN_STYLUS),
    NAME_ELEMENT(BTN_STYLUS2),      NAME_ELEMENT(BTN_TOOL_DOUBLETAP),
    NAME_ELEMENT(BTN_TOOL_TRIPLETAP),
#ifdef BTN_TOOL_QUADTAP
    NAME_ELEMENT(BTN_TOOL_QUADTAP),
#endif
    NAME_ELEMENT(BTN_GEAR_DOWN),
    NAME_ELEMENT(BTN_GEAR_UP),

#ifdef BTN_DPAD_UP
    NAME_ELEMENT(BTN_DPAD_UP),      NAME_ELEMENT(BTN_DPAD_DOWN),
    NAME_ELEMENT(BTN_DPAD_LEFT),        NAME_ELEMENT(BTN_DPAD_RIGHT),
#endif
#ifdef KEY_ALS_TOGGLE
    NAME_ELEMENT(KEY_ALS_TOGGLE),
#endif
#ifdef KEY_BUTTONCONFIG
    NAME_ELEMENT(KEY_BUTTONCONFIG),
#endif
#ifdef KEY_TASKMANAGER
    NAME_ELEMENT(KEY_TASKMANAGER),
#endif
#ifdef KEY_JOURNAL
    NAME_ELEMENT(KEY_JOURNAL),
#endif
#ifdef KEY_CONTROLPANEL
    NAME_ELEMENT(KEY_CONTROLPANEL),
#endif
#ifdef KEY_APPSELECT
    NAME_ELEMENT(KEY_APPSELECT),
#endif
#ifdef KEY_SCREENSAVER
    NAME_ELEMENT(KEY_SCREENSAVER),
#endif
#ifdef KEY_VOICECOMMAND
    NAME_ELEMENT(KEY_VOICECOMMAND),
#endif
#ifdef KEY_BRIGHTNESS_MIN
    NAME_ELEMENT(KEY_BRIGHTNESS_MIN),
#endif
#ifdef KEY_BRIGHTNESS_MAX
    NAME_ELEMENT(KEY_BRIGHTNESS_MAX),
#endif
#ifdef KEY_KBDINPUTASSIST_PREV
    NAME_ELEMENT(KEY_KBDINPUTASSIST_PREV),
#endif
#ifdef KEY_KBDINPUTASSIST_NEXT
    NAME_ELEMENT(KEY_KBDINPUTASSIST_NEXT),
#endif
#ifdef KEY_KBDINPUTASSIST_PREVGROUP
    NAME_ELEMENT(KEY_KBDINPUTASSIST_PREVGROUP),
#endif
#ifdef KEY_KBDINPUTASSIST_NEXTGROUP
    NAME_ELEMENT(KEY_KBDINPUTASSIST_NEXTGROUP),
#endif
#ifdef KEY_KBDINPUTASSIST_ACCEPT
    NAME_ELEMENT(KEY_KBDINPUTASSIST_ACCEPT),
#endif
#ifdef KEY_KBDINPUTASSIST_CANCEL
    NAME_ELEMENT(KEY_KBDINPUTASSIST_CANCEL),
#endif
#ifdef BTN_TRIGGER_HAPPY
    NAME_ELEMENT(BTN_TRIGGER_HAPPY1),   NAME_ELEMENT(BTN_TRIGGER_HAPPY11),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY2),   NAME_ELEMENT(BTN_TRIGGER_HAPPY12),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY3),   NAME_ELEMENT(BTN_TRIGGER_HAPPY13),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY4),   NAME_ELEMENT(BTN_TRIGGER_HAPPY14),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY5),   NAME_ELEMENT(BTN_TRIGGER_HAPPY15),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY6),   NAME_ELEMENT(BTN_TRIGGER_HAPPY16),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY7),   NAME_ELEMENT(BTN_TRIGGER_HAPPY17),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY8),   NAME_ELEMENT(BTN_TRIGGER_HAPPY18),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY9),   NAME_ELEMENT(BTN_TRIGGER_HAPPY19),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY10),  NAME_ELEMENT(BTN_TRIGGER_HAPPY20),

    NAME_ELEMENT(BTN_TRIGGER_HAPPY21),  NAME_ELEMENT(BTN_TRIGGER_HAPPY31),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY22),  NAME_ELEMENT(BTN_TRIGGER_HAPPY32),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY23),  NAME_ELEMENT(BTN_TRIGGER_HAPPY33),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY24),  NAME_ELEMENT(BTN_TRIGGER_HAPPY34),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY25),  NAME_ELEMENT(BTN_TRIGGER_HAPPY35),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY26),  NAME_ELEMENT(BTN_TRIGGER_HAPPY36),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY27),  NAME_ELEMENT(BTN_TRIGGER_HAPPY37),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY28),  NAME_ELEMENT(BTN_TRIGGER_HAPPY38),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY29),  NAME_ELEMENT(BTN_TRIGGER_HAPPY39),
    NAME_ELEMENT(BTN_TRIGGER_HAPPY30),  NAME_ELEMENT(BTN_TRIGGER_HAPPY40),
#endif
#ifdef BTN_TOOL_QUINTTAP
    NAME_ELEMENT(BTN_TOOL_QUINTTAP),
#endif
};

static const char * const absval[6] = { "Value", "Min  ", "Max  ", "Fuzz ", "Flat ", "Resolution "};

static const char * const relatives[REL_MAX + 1] = {
    [0 ... REL_MAX] = NULL,
    NAME_ELEMENT(REL_X),            NAME_ELEMENT(REL_Y),
    NAME_ELEMENT(REL_Z),            NAME_ELEMENT(REL_RX),
    NAME_ELEMENT(REL_RY),           NAME_ELEMENT(REL_RZ),
    NAME_ELEMENT(REL_HWHEEL),
    NAME_ELEMENT(REL_DIAL),         NAME_ELEMENT(REL_WHEEL),
    NAME_ELEMENT(REL_MISC),
};

static const char * const absolutes[ABS_MAX + 1] = {
    [0 ... ABS_MAX] = NULL,
    NAME_ELEMENT(ABS_X),            NAME_ELEMENT(ABS_Y),
    NAME_ELEMENT(ABS_Z),            NAME_ELEMENT(ABS_RX),
    NAME_ELEMENT(ABS_RY),           NAME_ELEMENT(ABS_RZ),
    NAME_ELEMENT(ABS_THROTTLE),     NAME_ELEMENT(ABS_RUDDER),
    NAME_ELEMENT(ABS_WHEEL),        NAME_ELEMENT(ABS_GAS),
    NAME_ELEMENT(ABS_BRAKE),        NAME_ELEMENT(ABS_HAT0X),
    NAME_ELEMENT(ABS_HAT0Y),        NAME_ELEMENT(ABS_HAT1X),
    NAME_ELEMENT(ABS_HAT1Y),        NAME_ELEMENT(ABS_HAT2X),
    NAME_ELEMENT(ABS_HAT2Y),        NAME_ELEMENT(ABS_HAT3X),
    NAME_ELEMENT(ABS_HAT3Y),        NAME_ELEMENT(ABS_PRESSURE),
    NAME_ELEMENT(ABS_DISTANCE),     NAME_ELEMENT(ABS_TILT_X),
    NAME_ELEMENT(ABS_TILT_Y),       NAME_ELEMENT(ABS_TOOL_WIDTH),
    NAME_ELEMENT(ABS_VOLUME),       NAME_ELEMENT(ABS_MISC),
#ifdef ABS_MT_BLOB_ID
    NAME_ELEMENT(ABS_MT_TOUCH_MAJOR),
    NAME_ELEMENT(ABS_MT_TOUCH_MINOR),
    NAME_ELEMENT(ABS_MT_WIDTH_MAJOR),
    NAME_ELEMENT(ABS_MT_WIDTH_MINOR),
    NAME_ELEMENT(ABS_MT_ORIENTATION),
    NAME_ELEMENT(ABS_MT_POSITION_X),
    NAME_ELEMENT(ABS_MT_POSITION_Y),
    NAME_ELEMENT(ABS_MT_TOOL_TYPE),
    NAME_ELEMENT(ABS_MT_BLOB_ID),
#endif
#ifdef ABS_MT_TRACKING_ID
    NAME_ELEMENT(ABS_MT_TRACKING_ID),
#endif
#ifdef ABS_MT_PRESSURE
    NAME_ELEMENT(ABS_MT_PRESSURE),
#endif
#ifdef ABS_MT_SLOT
    NAME_ELEMENT(ABS_MT_SLOT),
#endif
#ifdef ABS_MT_TOOL_X
    NAME_ELEMENT(ABS_MT_TOOL_X),
    NAME_ELEMENT(ABS_MT_TOOL_Y),
    NAME_ELEMENT(ABS_MT_DISTANCE),
#endif

};

static const char * const misc[MSC_MAX + 1] = {
    [ 0 ... MSC_MAX] = NULL,
    NAME_ELEMENT(MSC_SERIAL),       NAME_ELEMENT(MSC_PULSELED),
    NAME_ELEMENT(MSC_GESTURE),      NAME_ELEMENT(MSC_RAW),
    NAME_ELEMENT(MSC_SCAN),
#ifdef MSC_TIMESTAMP
    NAME_ELEMENT(MSC_TIMESTAMP),
#endif
};

static const char * const leds[LED_MAX + 1] = {
    [0 ... LED_MAX] = NULL,
    NAME_ELEMENT(LED_NUML),         NAME_ELEMENT(LED_CAPSL),
    NAME_ELEMENT(LED_SCROLLL),      NAME_ELEMENT(LED_COMPOSE),
    NAME_ELEMENT(LED_KANA),         NAME_ELEMENT(LED_SLEEP),
    NAME_ELEMENT(LED_SUSPEND),      NAME_ELEMENT(LED_MUTE),
    NAME_ELEMENT(LED_MISC),
#ifdef LED_MAIL
    NAME_ELEMENT(LED_MAIL),
#endif
#ifdef LED_CHARGING
    NAME_ELEMENT(LED_CHARGING),
#endif
};

static const char * const repeats[REP_MAX + 1] = {
    [0 ... REP_MAX] = NULL,
    NAME_ELEMENT(REP_DELAY),        NAME_ELEMENT(REP_PERIOD)
};

static const char * const sounds[SND_MAX + 1] = {
    [0 ... SND_MAX] = NULL,
    NAME_ELEMENT(SND_CLICK),        NAME_ELEMENT(SND_BELL),
    NAME_ELEMENT(SND_TONE)
};

static const char * const syns[SYN_MAX + 1] = {
    [0 ... SYN_MAX] = NULL,
    NAME_ELEMENT(SYN_REPORT),
    NAME_ELEMENT(SYN_CONFIG),
    NAME_ELEMENT(SYN_MT_REPORT),
    NAME_ELEMENT(SYN_DROPPED)
};

static const char * const switches[SW_MAX + 1] = {
    [0 ... SW_MAX] = NULL,
    NAME_ELEMENT(SW_LID),
    NAME_ELEMENT(SW_TABLET_MODE),
    NAME_ELEMENT(SW_HEADPHONE_INSERT),
#ifdef SW_RFKILL_ALL
    NAME_ELEMENT(SW_RFKILL_ALL),
#endif
#ifdef SW_MICROPHONE_INSERT
    NAME_ELEMENT(SW_MICROPHONE_INSERT),
#endif
#ifdef SW_DOCK
    NAME_ELEMENT(SW_DOCK),
#endif
#ifdef SW_LINEOUT_INSERT
    NAME_ELEMENT(SW_LINEOUT_INSERT),
#endif
#ifdef SW_JACK_PHYSICAL_INSERT
    NAME_ELEMENT(SW_JACK_PHYSICAL_INSERT),
#endif
#ifdef SW_VIDEOOUT_INSERT
    NAME_ELEMENT(SW_VIDEOOUT_INSERT),
#endif
#ifdef SW_CAMERA_LENS_COVER
    NAME_ELEMENT(SW_CAMERA_LENS_COVER),
    NAME_ELEMENT(SW_KEYPAD_SLIDE),
    NAME_ELEMENT(SW_FRONT_PROXIMITY),
#endif
#ifdef SW_ROTATE_LOCK
    NAME_ELEMENT(SW_ROTATE_LOCK),
#endif
#ifdef SW_LINEIN_INSERT
    NAME_ELEMENT(SW_LINEIN_INSERT),
#endif
#ifdef SW_MUTE_DEVICE
    NAME_ELEMENT(SW_MUTE_DEVICE),
#endif
};

static const char * const force[FF_MAX + 1] = {
    [0 ... FF_MAX] = NULL,
    NAME_ELEMENT(FF_RUMBLE),        NAME_ELEMENT(FF_PERIODIC),
    NAME_ELEMENT(FF_CONSTANT),      NAME_ELEMENT(FF_SPRING),
    NAME_ELEMENT(FF_FRICTION),      NAME_ELEMENT(FF_DAMPER),
    NAME_ELEMENT(FF_INERTIA),       NAME_ELEMENT(FF_RAMP),
    NAME_ELEMENT(FF_SQUARE),        NAME_ELEMENT(FF_TRIANGLE),
    NAME_ELEMENT(FF_SINE),          NAME_ELEMENT(FF_SAW_UP),
    NAME_ELEMENT(FF_SAW_DOWN),      NAME_ELEMENT(FF_CUSTOM),
    NAME_ELEMENT(FF_GAIN),          NAME_ELEMENT(FF_AUTOCENTER),
};

static const char * const forcestatus[FF_STATUS_MAX + 1] = {
    [0 ... FF_STATUS_MAX] = NULL,
    NAME_ELEMENT(FF_STATUS_STOPPED),    NAME_ELEMENT(FF_STATUS_PLAYING),
};

static const char * const * const names[EV_MAX + 1] = {
    [0 ... EV_MAX] = NULL,
    [EV_SYN] = syns,            [EV_KEY] = keys,
    [EV_REL] = relatives,           [EV_ABS] = absolutes,
    [EV_MSC] = misc,            [EV_LED] = leds,
    [EV_SND] = sounds,          [EV_REP] = repeats,
    [EV_SW] = switches,
    [EV_FF] = force,            [EV_FF_STATUS] = forcestatus,
};

static void dump_input_device(void) {
    struct list_head* pos;

    pthread_mutex_lock(&device_list_lock);

    LOGD("========================================\n");
    LOGD("Dump input devices\n");
    list_for_each(pos, &input_dev_list) {
        struct input_device* device = list_entry(pos,
                struct input_device, head);

        LOGD("----------------------------------------\n");
        LOGD("name: %s\n", device->name);
        LOGD("path: %s\n", device->dev_path);
    }
    LOGD("========================================\n");

    pthread_mutex_unlock(&device_list_lock);
}


static inline const char* typename(unsigned int type) {
    return (type <= EV_MAX && events[type]) ? events[type] : "?";
}

static inline const char* codename(unsigned int type, unsigned int code) {
    return (type <= EV_MAX && code <= maxval[type] && names[type] &&
            names[type][code]) ? names[type][code] : "?";
}

static int is_event_device(const struct dirent *dir) {
    return strncmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
}

static int scan_devices(void) {
    struct dirent **namelist;
    int ndev, devnum;

    pthread_mutex_lock(&device_list_lock);

    ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);
    if (ndev <= 0)
        return -1;

    for (int i = 0; i < ndev; i++) {
        char fname[64];
        int fd = -1;
        char name[256] = "???";

        snprintf(fname, sizeof(fname),
             "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);

        fd = open(fname, O_RDONLY);
        if (fd < 0)
            continue;

        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        sscanf(namelist[i]->d_name, "event%d", &devnum);

        if (fd > max_fd)
            max_fd = fd;

        free(namelist[i]);

        struct input_device* device = calloc(1, sizeof(struct input_device));

        strcpy(device->name, name);
        strcpy(device->dev_path, fname);
        device->fd = fd;

        list_add_tail(&device->head, &input_dev_list);
    }

    pthread_mutex_unlock(&device_list_lock);

    return 0;
}

static void dump_event(struct input_event* event) {
    LOGI("========================================\n");
    LOGI("Dump input event\n");
    LOGI("time:  %ld.%06ld\n", event->time.tv_sec, event->time.tv_usec);
    LOGI("type:  %s\n", typename(event->type));
    LOGI("code:  %s\n", codename(event->type, event->code));
    LOGI("value: 0x%x\n", event->value);
    LOGI("========================================\n");
}

static void on_event(struct input_event* event) {
    struct list_head* pos;

    pthread_mutex_lock(&listener_lock);

    list_for_each(pos, &listener_list) {
        struct listener* l = list_entry(pos, struct listener, head);
        l->cb(event);
    }

    pthread_mutex_unlock(&listener_lock);
}

static void* thread_loop(void *param) {
    int readed = 0;
    int error = 0;
    fd_set rdfs;
    struct input_event ev[256];

    FD_ZERO(&rdfs);

    if (max_fd < local_pipe[0])
        max_fd = local_pipe[0];

    struct list_head* pos;
    list_for_each(pos, &input_dev_list) {
        struct input_device* device = list_entry(pos, struct input_device, head);
        FD_SET(device->fd, &rdfs);
    }

    for (;;) {
        error = select(max_fd + 1, &rdfs, NULL, NULL, NULL);
        if (error <= 0) {
            LOGE("select error, continue.\n");
            continue;
        }

        struct list_head* pos;

        pthread_mutex_lock(&device_list_lock);

        list_for_each(pos, &input_dev_list) {
            struct input_device* device = list_entry(pos, struct input_device, head);
            if (FD_ISSET(device->fd, &rdfs)) {
                readed = read(device->fd, ev, sizeof(ev));
                if (readed < (int)(sizeof(struct input_event))) {
                    LOGE("read error\n");
                    continue;
                }

                for (int i = 0; i < readed / sizeof(struct input_event); i++)
                    on_event(&ev[i]);
            }
        }
        pthread_mutex_unlock(&device_list_lock);

        if (FD_ISSET(local_pipe[0], &rdfs)) {
            char c;
            error = read(local_pipe[0], &c, 1);
            if (!error) {
                 LOGE("Unable to read pipe: %s\n", strerror(errno));
                 continue;
             }

            LOGI("main thread call me break out\n");
            break;
        }
    }

    return NULL;
}

static int start(void) {
    int error  = 0;

    pthread_mutex_lock(&start_lock);
    if (started == 1) {
        LOGE("netlink listener already start listener\n");
        pthread_mutex_unlock(&start_lock);
        return -1;
    }

    error = pipe(local_pipe);
    if (error) {
        LOGE("Unable to open pipe: %s\n", strerror(errno));
        pthread_mutex_unlock(&start_lock);
        return -1;
    }

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    error = pthread_create(&tid, &attr, thread_loop, NULL);
    if (error) {
        LOGE("pthread_create failed: %s\n", strerror(errno));
        pthread_attr_destroy(&attr);
        pthread_mutex_unlock(&start_lock);
        return -1;
    }

    pthread_attr_destroy(&attr);

    started = 1;

    pthread_mutex_unlock(&start_lock);

    return 0;
}

static int stop(void) {
    char c = 0;

    pthread_mutex_lock(&start_lock);
    if (started == 0) {
        LOGE("netlink listener already stop listener\n");
        goto error;
    }

    if (!write(local_pipe[1], &c, 1)) {
        LOGE("Unable to write pipe: %s\n", strerror(errno));
        goto error;
    }

    started = 0;

    pthread_mutex_unlock(&start_lock);
    return 0;

error:
    pthread_mutex_unlock(&start_lock);

    return -1;
}

static int init(void) {
    int error = 0;

    pthread_mutex_lock(&init_lock);

    if (inited == 1) {
        LOGE("battery manager already init\n");
        goto error;
    }

    error = scan_devices();
    if (error < 0) {
        LOGE("Failed to scan input device\n");
        goto error;
    }

    inited = 1;

    pthread_mutex_unlock(&init_lock);

    dump_input_device();

    return 0;

error:
    pthread_mutex_unlock(&init_lock);

    return -1;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (inited == 0) {
        LOGE("battery manager already deinit\n");
        pthread_mutex_unlock(&init_lock);
        return -1;
    }
    inited = 0;

    pthread_mutex_lock(&device_list_lock);
    struct list_head* pos;
    struct list_head* next_pos;
    list_for_each_safe(pos, next_pos, &input_dev_list) {
        struct input_device* device = list_entry(pos, struct input_device, head);

        list_del(&device->head);
        free(device);
    }
    pthread_mutex_unlock(&device_list_lock);

    if (local_pipe[0] > 0)
        close(local_pipe[0]);

    if (local_pipe[1] > 0)
        close(local_pipe[1]);

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static int get_device_count(void) {
    pthread_mutex_lock(&device_list_lock);

    int size = get_list_size_locked(&input_dev_list);

    pthread_mutex_unlock(&device_list_lock);

    return size;
}

static void register_event_listener(input_event_listener_t listener) {
    assert_die_if(listener == NULL, "listener is NULL\n");

    struct list_head* pos;
    pthread_mutex_lock(&listener_lock);

    list_for_each(pos, &listener_list) {
        struct listener *l = list_entry(pos, struct listener, head);
        if (l->cb == listener) {
            pthread_mutex_unlock(&listener_lock);
            return;
        }
    }

    struct listener *l = malloc(sizeof(struct listener));
    l->cb = listener;
    list_add_tail(&l->head, &listener_list);

    pthread_mutex_unlock(&listener_lock);
}

static void unregister_event_listener(input_event_listener_t listener) {
    assert_die_if(listener == NULL, "listener is NULL\n");

    struct list_head* pos;
    struct list_head* next_pos;

    pthread_mutex_lock(&listener_lock);

    list_for_each_safe(pos, next_pos, &listener_list) {
        struct listener* l = list_entry(pos, struct listener, head);

        if (l->cb == listener) {
            list_del(&l->head);

            free(l);

            pthread_mutex_unlock(&listener_lock);

            return;
        }
    }

    pthread_mutex_unlock(&listener_lock);
}

static struct input_manager this = {
        .init = init,
        .deinit = deinit,
        .start = start,
        .stop = stop,
        .get_devices_count = get_device_count,
        .register_event_listener = register_event_listener,
        .unregister_event_listener = unregister_event_listener,
        .dump_event = dump_event,
        .type2str = typename,
        .code2str = codename,
};

struct input_manager* get_input_manager(void) {
    return &this;
}
