/*
 * pwm/draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_DRAW_H
#define INCLUDED_DRAW_H

#include "common.h"

struct _WFrame;
struct _WMenu;
struct _WClientWin;
struct _WDock;

enum{
	WCG_PIX_BG=0,
	WCG_PIX_HL=1,
	WCG_PIX_SH=2,
	WCG_PIX_FG=3,
	WCG_PIX_N=4
};


typedef struct _WColorGroup{
	ulong pixels[WCG_PIX_N];
} WColorGroup;


typedef struct _WGRData{
	/* configurable data */
	XFontStruct *font, *menu_font;
	
	int border_width;
	int bevel_width;
	
	int bar_min_width;
	float bar_max_width_q;
	int tab_min_width;
		
	WColorGroup act_tab_colors, act_tab_sel_colors;
	WColorGroup act_base_colors, act_sel_colors;
	WColorGroup tab_colors, tab_sel_colors;
	WColorGroup base_colors, sel_colors;
	
	/* other data */
	int bar_height;
	int submenu_ind_w;
	
	GC gc;
	GC xor_gc;
	GC stipple_gc;
	GC menu_gc;
	GC copy_gc;
	Pixmap stick_pixmap;
	int stick_pixmap_w;
	int stick_pixmap_h;
} WGRData;


extern void draw_moveres(const char *str);
extern void draw_tabdrag(const struct _WClientWin *cwin);

extern void draw_frame_frame(const struct _WFrame *frame, bool complete);
extern void draw_frame_bar(const struct _WFrame *frame, bool complete);
extern void draw_frame(const struct _WFrame *frame, bool complete);
extern void draw_menu(const struct _WMenu *menu, bool complete);
extern void draw_menu_selection(const struct _WMenu *menu);
extern void erase_menu_selection(const struct _WMenu *menu);
extern void draw_dock(const struct _WDock *dock, bool complete);

extern void draw_rubberband(const struct _WWinObj *obj,
							int x, int y, int w, int h);

#endif /* INCLUDED_DRAW_H */
