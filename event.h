/*
 * pwm/event.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_EVENT_H
#define INCLUDED_EVENT_H

#include "common.h"

#define GRAB_POINTER_MASK (ButtonPressMask|ButtonReleaseMask|\
						   ButtonMotionMask)

#define GRAB_KEY_MASK (KeyPressMask|KeyReleaseMask)

#define ROOT_MASK	(SubstructureRedirectMask|          \
					 ColormapChangeMask|                \
					 ButtonPressMask|ButtonReleaseMask| \
					 PropertyChangeMask|KeyPressMask|   \
					 FocusChangeMask|EnterWindowMask)

#define BAR_MASK	(FocusChangeMask|   \
					 ButtonPressMask|   \
					 ButtonReleaseMask| \
					 KeyPressMask|      \
					 EnterWindowMask|   \
					 ExposureMask)

#define FRAME_MASK 	(SubstructureRedirectMask|BAR_MASK)
#define MENU_MASK 	BAR_MASK
#define DOCK_MASK 	BAR_MASK

#define CLIENT_MASK (ColormapChangeMask/*|EnterWindowMask*/| \
					 PropertyChangeMask|FocusChangeMask| \
					 StructureNotifyMask)


extern void mainloop();
extern void get_event(XEvent *ev);
extern void get_event_mask(XEvent *ev, long mask);
extern void handle_event(XEvent *ev);

extern void grab_kb_ptr();
extern void ungrab_kb_ptr();

#endif /* INCLUDED_EVENT_H */
