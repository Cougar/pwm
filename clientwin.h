/*
 * pwm/clientwin.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_CLIENTWIN_H
#define INCLUDED_CLIENTWIN_H

#include "common.h"
#include "thing.h"

#define CWIN_DRAG 				0x0001
#define CWIN_TAGGED				0x0002
#define CWIN_URGENT				0x0004
#define CWIN_WILD				0x0008
#define CWIN_P_WM_DELETE 		0x0100
#define CWIN_P_WM_TAKE_FOCUS 	0x0200

#define CWIN_IS_SELECTED(CWIN)	((CWIN)->flags&CWIN_SELECTED)
#define CWIN_IS_WILD(CWIN) 		((CWIN)->flags&CWIN_WILD)

#define MANAGE_RESPECT_POS		0x0001
#define MANAGE_INITIAL			0x0002

struct _WFrame;

typedef struct _WClientWin{
	INHERIT_WTHING;
	
	int state;
	long event_mask;
	int dockpos;
	
	Window client_win;
	int client_h, client_w;
	int orig_bw;
	
	Window transient_for;
	XSizeHints size_hints;
	
	Colormap cmap;
	Colormap *cmaps;
	Window *cmapwins;
	int n_cmapwins;
	
	char *name;
	char *icon_name;
	char *label;
	int label_inst;
	struct _WClientWin *label_next, *label_prev;
	int label_width;
	
	struct _WClientWin *s_cwin_next;
	struct _WClientWin *s_cwin_prev;
} WClientWin;


#define CWIN_FRAME(CWIN) ((WFrame*)((CWIN)->t_parent))
#define CWIN_HAS_FRAME(CWIN) clientwin_has_frame(CWIN)


extern WClientWin *manage_clientwin(Window win, int mflags);
extern WClientWin* create_clientwin(Window win, int flags, int initial_state,
									const XWindowAttributes *attr);

extern void unmap_clientwin(WClientWin *cwin);
extern void unmanage_clientwin(WClientWin *cwin);
extern void destroy_clientwin(WClientWin *cwin);
extern void iconify_clientwin(WClientWin *cwin);
extern void kill_clientwin(WClientWin *cwin);
extern void close_clientwin(WClientWin *cwin);

extern void hide_clientwin(WClientWin *cwin);
extern void show_clientwin(WClientWin *cwin);

extern void get_clientwin_size_hints(WClientWin *cwin);
extern void set_clientwin_size(WClientWin *cwin, int w, int h);
extern void clientwin_reconf_at(WClientWin *cwin, int rootx, int rooty);
extern void send_clientmsg(Window win, Atom a);

extern void clientwin_toggle_tagged(WClientWin *cwin);

extern bool clientwin_has_frame(WClientWin *cwin);
extern void clientwin_detach(WClientWin *cwin);

extern void attachdetach_clientwin(struct _WFrame *frame, WClientWin *cwin,
								   bool coords, int x, int y);

extern void install_cmap(Colormap cmap);
extern void install_cwin_cmap(WClientWin *cwin);

extern void clientwin_use_label(WClientWin *cwin);
extern void clientwin_unuse_label(WClientWin *cwin);
extern void clientwin_make_label(WClientWin *cwin, int maxw);
extern char *clientwin_full_label(WClientWin *cwin);

#endif /* INCLUDED_CLIENTWIN_H */
