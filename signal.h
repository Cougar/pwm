/*
 * pwm/signal.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
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
