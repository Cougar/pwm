/*
 * pwm/pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_POINTER_H
#define INCLUDED_POINTER_H

#include "common.h"
#include "clientwin.h"
#include "function.h"
#include "menu.h"


enum{
	DRAG_MOVE,
	DRAG_RESIZE,
	DRAG_MOVE_STEPPED,
	DRAG_RESIZE_STEPPED,
	DRAG_TAB
};


enum{
	POINTER_NORMAL,
	POINTER_MENU,
	POINTER_MENU_MOVE
};


/* */


extern void handle_button_press(XButtonEvent *ev);
extern bool handle_button_release(XButtonEvent *ev);
extern void handle_pointer_motion(XMotionEvent *ev);
extern void get_pointer_rootpos(int *xret, int *yret);
extern void pointer_change_context(WThing *thing, uint actx);
extern bool find_window_at(int x, int y, Window *childret);

#endif /* INCLUDED_POINTER_H */
