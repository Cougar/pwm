/*
 * pwm/cursor.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_CURSOR_H
#define INCLUDED_CURSOR_H

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#define CURSOR_DEFAULT 	0
#define CURSOR_RESIZE 	1
#define CURSOR_MOVE 	2
#define CURSOR_DRAG		3
#define N_CURSORS		4

extern void load_cursors();
extern void change_grab_cursor(int cursor);
extern void set_cursor(Window win, int cursor);
extern Cursor x_cursor(int cursor);

#endif /* INCLUDED_CURSOR_H */
