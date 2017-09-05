/*
 *  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
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

#include <signal.h>
#include <stdio.h>

#include <signal/signal_handler.h>

#define LOG_TAG "signal_handler"

static void set_signal_handler(struct signal_handler* this, int signal,
        signal_handler_t handler) {

    if (handler == NULL)
        this->action.sa_handler = SIG_IGN;
    else
        this->action.sa_handler = handler;

    sigaction(signal, &this->action, NULL);
}

void construct_signal_handler(struct signal_handler* this) {
    sigemptyset(&this->action.sa_mask);
    this->action.sa_flags = 0;

    this->set_signal_handler = set_signal_handler;
}

void destruct_signal_handler(struct signal_handler* this) {
    this->set_signal_handler = NULL;
}
