/*
 * pwm/screen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

struct _WScreen;

#ifndef INCLUDED_SCREEN_H
#define INCLUDED_SCREEN_H

#include "common.h"
#include "font.h"
#include "winobj.h"
#include "draw.h"


struct _WDock;


typedef struct _WScreen{
	INHERIT_WTHING;
	
	int xscr;
	int width, height;
	Window root;
	Colormap default_cmap;
	
	Window moveres_win;
	Window tabdrag_win;
	int moveres_win_w, moveres_win_h;
	
	struct _WWinObj *winobj_stack_lists[N_STACK_LVLS];
	struct _WClientWin *clientwin_list;
	int n_clientwin;
	
	int opaque_move;
	
	int n_workspaces;
	int workspaces_horiz, workspaces_vert;
	int current_workspace;
	
	struct _WDock *dock;
} WScreen;


extern bool preinit_screen(int xscr);
extern void postinit_screen();
extern void deinit_screen();
extern Window create_simple_window(int x, int y, int w, int h,
								   ulong background);


extern bool alloc_color(const char *name, ulong *cret);

#include "workspace.h"
#include "dock.h"

#endif /* INCLUDED_SCREEN_H */
