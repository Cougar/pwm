/*
 * pwm/global.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_GLOBAL_H
#define INCLUDED_GLOBAL_H

#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include "common.h"
#include "screen.h"
#include "clientwin.h"
#include "winobj.h"
#include "menu.h"
#include "draw.h"

#define MAX_SCREENS 1

enum{
	INPUT_NORMAL,
	INPUT_MOVERES,
	INPUT_CTXMENU
};

typedef struct _WGlobal{
	int argc;
	char **argv;
	
	Display *dpy;
	const char *display;
	int conn;
	
	int n_children;
	int n_alive;
	pid_t *children;
	pid_t parent;
	
	WScreen screen;
	WGRData grdata;
	
	XContext win_context;
	Atom atom_wm_state;
	Atom atom_wm_change_state;
	Atom atom_wm_protocols;
	Atom atom_wm_delete;
	Atom atom_wm_take_focus;
	Atom atom_wm_colormaps;
	Atom atom_frame_id;
	Atom atom_workspace_num;
	Atom atom_workspace_info;
	Atom atom_private_ipc;
#ifndef CF_NO_MWM_HINTS	
	Atom atom_mwm_hints;
#endif

	WWinObj *current_winobj, *previous_winobj;
	WThing *grab_holder;
	WThing *focus_next;
	int input_mode;

	Time dblclick_delay;
} WGlobal;

extern WGlobal wglobal;

extern void deinit();

#endif /* INCLUDED_GLOBAL_H */
