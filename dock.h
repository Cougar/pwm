/*
 * pwm/dock.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_DOCK_H
#define INCLUDED_DOCK_H

#include "common.h"
#include "winobj.h"

#define DOCK_HORIZONTAL	0x0001
#define DOCK_RIGHT 		0x0002
#define DOCK_BOTTOM		0x0004
#define DOCK_SLIDING	0x0008

struct _WClientWin;

typedef struct _WDock{
	INHERIT_WWINOBJ;
	
	Window win;
	
	int dockwin_count;
	int max_vis_dockwin;
	int dock_w, dock_h;
} WDock;

extern void set_dock_params(const char *geom, const uint flags);
extern bool add_dockwin(struct _WClientWin *cwin);
extern void remove_dockwin(struct _WClientWin *cwin);
extern struct _WClientWin *dockwin_at(WDock *dock, int x, int y);
extern void destroy_dock(WDock *dock);
extern void set_dock_pos(WDock *dock, int x, int y);
extern void dock_toggle_hide();
extern void dock_toggle_dir();

#endif /* INCLUDED_DOCK_H */
