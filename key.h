/*
 * pwm/key.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_KEY_H
#define INCLUDED_KEY_H

#include <X11/keysym.h>
#include "function.h"

extern void handle_keypress(XKeyEvent *ev);

#endif /* INCLUDED_KEY_H */
