/*
 * pwm/signal.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_SIGNAL_H
#define INCLUDED_SIGNAL_H

#include "common.h"

#define COMM_WIN DefaultRootWindow(wglobal.dpy)

extern void check_signals();
extern void trap_signals();
extern void set_timer(uint msecs, void (*handler)());
extern void reset_timer();

#endif /* INCLUDED_SIGNAL_H */
