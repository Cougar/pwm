/*
 * pwm/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_FOCUS_H
#define INCLUDED_FOCUS_H

#include "thing.h"
#include "screen.h"
#include "winobj.h"

extern void set_focus(WThing *thing);
extern void set_focus_weak(WThing *thing);
extern void do_set_focus(WThing *thing);
extern WWinObj* circulate(int dir);
extern void circulateraise(int dir);
extern void goto_previous();

#endif /* INCLUDED_FOCUS_H */
