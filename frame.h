/*
 * pwm/frame.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_INCLUDED_FRAME_H
#define INCLUDED_INCLUDED_FRAME_H

#include "common.h"
#include "winobj.h"


#define WFRAME_HIDDEN		WWINOBJ_HIDDEN
#define WFRAME_MAX_VERT 	0x0001
#define WFRAME_MAX_HORIZ	0x0002
#define WFRAME_SHADE		0x0004
#define WFRAME_NO_BAR		0x0008
#define WFRAME_NO_BORDER	0x0010

#define WFRAME_NO_DECOR		(WFRAME_NO_BAR|WFRAME_NO_BORDER)
#define WFRAME_MAX_BOTH		(WFRAME_MAX_VERT|WFRAME_MAX_HORIZ)

#define WFRAME_IS_HIDDEN(F) WWINOBJ_IS_HIDDEN(F)
#define WFRAME_IS_NO_BAR(F) ((F)->flags&WFRAME_NO_BAR)
#define WFRAME_IS_NO_BORDER(F) ((F)->flags&WFRAME_NO_BORDER)
#define WFRAME_IS_SHADE(F) 	((F)->flags&WFRAME_SHADE)
#define WFRAME_IS_NORMAL(F)	(!WFRAME_IS_HIDDEN(F) && !WFRAME_IS_SHADE(F))

#define BAR_X(FRAME) ((FRAME)->x)
#define BAR_Y(FRAME) ((FRAME)->y)
#define BAR_W(FRAME) ((FRAME)->bar_w)
#define BAR_H(FRAME) ((FRAME)->bar_h)
#define FRAME_X(FRAME) ((FRAME)->x)
#define FRAME_Y(FRAME) ((FRAME)->y+(FRAME)->bar_h)
#define FRAME_W(FRAME) ((FRAME)->w)
#define FRAME_H(FRAME) ((FRAME)->h-(FRAME)->bar_h)

#define FRAME_MAXW_UNSET(FRAME) ((FRAME)->max_w<=0)
#define FRAME_MAXH_UNSET(FRAME) ((FRAME)->max_h<=0)

#define FRAME_CWIN_LIST(FRAME) ((WClientWin*)((FRAME)->t_children))


/* */


struct _WClientWin;
struct _WScreen;

typedef struct _WFrame{
	INHERIT_WWINOBJ;

	int frame_id;

	/* Internal (client window) width and height and */
	/* X and Y indent of client window within frame window */
	int frame_iw, frame_ih, frame_ix, frame_iy;

	/* Bar height and width */
	int bar_w, bar_h, tab_w;

	Window frame_win, bar_win;

	int min_h, min_w;
	int max_h, max_w;

	int saved_iw, saved_ih;
	int saved_x, saved_y;

	int cwin_count;
	struct _WClientWin *current_cwin;
} WFrame;


/* */


extern WFrame *create_frame(int x, int y, int iw, int ih, int id, int flags);
extern WFrame *create_add_frame_simple(int x, int y, int iiw, int iih);

extern void destroy_frame(WFrame *frame);

extern bool frame_attach_clientwin(WFrame *frame, struct _WClientWin *cwin);
extern void frame_detach_clientwin(WFrame *frame, struct _WClientWin *cwin,
								   int x, int y);
extern void frame_attach_tagged(WFrame *frame);
extern void join_tagged();

extern void frame_switch_clientwin(WFrame *frame, struct _WClientWin *cwin);
extern void frame_switch_nth(WFrame *frame, int cwinnum);
extern void frame_switch_rot(WFrame *frame, int rotcnt);

extern void activate_frame(WFrame *frame);
extern void deactivate_frame(WFrame *frame);

extern void set_frame_size(WFrame *frame, int w, int h);
extern void set_frame_pos(WFrame *frame, int x, int y);
extern void set_frame_state(WFrame *frame, int stateflags);
extern void frame_clientwin_resize(WFrame *frame, struct _WClientWin *cwin,
								   int w, int h, bool dmax);

extern void frame_toggle_shade(WFrame *frame);
extern void frame_toggle_sticky(WFrame *frame);
extern void frame_toggle_maximize(WFrame *frame, int mask);

extern void frame_recalc_bar(WFrame *frame);
extern void frame_recalc_minmax(WFrame *frame);

extern void clientwin_to_frame_size(int iw, int ih, int flags,
									int *fw, int *fh);

extern void frame_set_decor(WFrame *frame, int decorflags);
extern void frame_toggle_decor(WFrame *frame);

#endif /* INCLUDED_FRAME_H */
