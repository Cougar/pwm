/*
 * pwm/draw.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include <X11/Xlib.h>
#include <string.h>

#include "common.h"
#include "screen.h"
#include "draw.h"
#include "config.h"
#include "font.h"
#include "clientwin.h"
#include "frame.h"
#include "menu.h"


static void draw_bevel(Window win, GC gc,
					   int x, int y, int w, int h,
					   int bw, int hl, int sh)
{
	XPoint points[3];
	int i=0;
	
	w--;
	h--;

	XSetForeground(wglobal.dpy, gc, hl);

	for(i=0; i<bw; i++){	
		points[0].x=x+i;		points[0].y=y+h-i+1;
		points[1].x=x+i;		points[1].y=y+i;
		points[2].x=x+w-i;		points[2].y=y+i;
	
		XDrawLines(wglobal.dpy, win, gc, points, 3, CoordModeOrigin);
	}

	
	XSetForeground(wglobal.dpy, gc, sh);

	for(i=0; i<bw; i++){	
		points[0].x=x+w-i;		points[0].y=y+i;
		points[1].x=x+w-i;		points[1].y=y+h-i;
		points[2].x=x+i+1;		points[2].y=y+h-i;
	
		XDrawLines(wglobal.dpy, win, gc, points, 3, CoordModeOrigin);
	}
}



static void draw_box(Window win, GC gc, const WColorGroup *colors,
					 int x, int y, int w, int h, bool fill)
{
	if(fill){
		XSetForeground(wglobal.dpy, gc, colors->pixels[WCG_PIX_BG]);	
		XFillRectangle(wglobal.dpy, win, gc, x, y, w, h);
	}

	draw_bevel(win, gc, x, y, w, h, CF_BEVEL_WIDTH,
			   colors->pixels[WCG_PIX_HL], colors->pixels[WCG_PIX_SH]);
}


/* */


static void draw_tab(WGRData *grdata, Window win,
					 const WColorGroup *colors,
					 int x, int y, int w, int h,
					 bool fill, const char *str, int str_w)
{
	GC gc=grdata->gc;
	
	draw_box(win, gc, colors, x, y, w, h, fill);

	if(str!=NULL){
		XSetForeground(wglobal.dpy, gc, colors->pixels[WCG_PIX_FG]);
		XDrawString(wglobal.dpy, win, gc, x+w/2-str_w/2,
					y+CF_TAB_TEXT_Y_OFF+FONT_BASELINE(grdata->font),
					str, strlen(str));
	}
}


static void draw_border(WGRData *grdata, Window win,
						const WColorGroup *colors,
						int x, int y, int w, int h)
{

	int sw=CF_BEVEL_WIDTH;
	int dw=CF_BORDER_WIDTH-CF_BEVEL_WIDTH;
	GC gc=grdata->gc;
	
	draw_bevel(win, gc, x, y, w, h, sw,
			   colors->pixels[WCG_PIX_HL], colors->pixels[WCG_PIX_SH]);
	draw_bevel(win, gc, x+dw, y+dw, w-dw*2, h-dw*2, sw,
			   colors->pixels[WCG_PIX_SH], colors->pixels[WCG_PIX_HL]); 
}


static void copy_masked(const WGRData *grdata, Drawable src, Drawable dst,
						int src_x, int src_y, int w, int h,
						int dst_x, int dst_y)
{

	XSetClipMask(wglobal.dpy, grdata->copy_gc, src);
	XSetClipOrigin(wglobal.dpy, grdata->copy_gc, dst_x, dst_y);
	XCopyPlane(wglobal.dpy, src, dst, grdata->copy_gc, src_x, src_y,
			   w, h, dst_x, dst_y, 1);
}


/* */


static void draw_xor_frame(int x, int y, int w, int h, int bar_h, int bar_w)
{
	XPoint fpts[5];
	XPoint bpts[4];
	WGRData *grdata=GRDATA;
	
	if(bar_w>w*CF_BAR_MAX_WIDTH_Q)
		bar_w=w*CF_BAR_MAX_WIDTH_Q;
	
	if(bar_h!=0){
		bpts[0].x=x;
		bpts[0].y=y+bar_h;
		bpts[1].x=x;
		bpts[1].y=y;
		bpts[2].x=x+bar_w;
		bpts[2].y=y;
		bpts[3].x=x+bar_w;
		bpts[3].y=y+bar_h;
		XDrawLines(wglobal.dpy, SCREEN->root, grdata->xor_gc, bpts, 4,
				   CoordModeOrigin);
	}

	fpts[0].x=x;
	fpts[0].y=y+bar_h;
	fpts[1].x=x+w;
	fpts[1].y=y+bar_h;
	fpts[2].x=x+w;
	fpts[2].y=y+h;
	fpts[3].x=x;
	fpts[3].y=y+h;
	fpts[4].x=x;
	fpts[4].y=y+bar_h;
	
	XDrawLines(wglobal.dpy, SCREEN->root, grdata->xor_gc, fpts, 5,
			   CoordModeOrigin);
}


void draw_rubberband(const WWinObj *obj, int x, int y, int w, int h)
{
	WFrame *frame;
	
	if(!WTHING_IS(obj, WTHING_FRAME)){
		draw_xor_frame(x, y, w, h, 0, 0);
	}else{
		frame=(WFrame*)obj;
		draw_xor_frame(x, y, w, h, /*w+frame->frame_ix*2, h+frame->frame_iy*2,*/
					   frame->bar_h, frame->bar_w);
	}
}


void draw_moveres(const char *str)
{
	WGRData *grdata=GRDATA;
	WScreen *scr=SCREEN;
	int w=XTextWidth(grdata->font, str, strlen(str));
	
/*	XClearWindow(wglobal.dpy, scr->moveres_win);*/
	
	XClearArea(wglobal.dpy, scr->moveres_win,
			   CF_BEVEL_WIDTH, CF_BEVEL_WIDTH,
			   scr->moveres_win_w-CF_BEVEL_WIDTH*2,
			   scr->moveres_win_h-CF_BEVEL_WIDTH*2, False);
	
	draw_tab(GRDATA, scr->moveres_win, &(grdata->tab_sel_colors),
			 0, 0, scr->moveres_win_w, scr->moveres_win_h,
			 FALSE, str, w);
}


void draw_tabdrag(const WClientWin *cwin)
{
	WFrame *frame;
	const WColorGroup *tabcolor;
	WGRData *grdata=GRDATA;
	Window win=SCREEN->tabdrag_win;
	
	frame=CWIN_FRAME(cwin);
	
	if((WWinObj*)frame==wglobal.current_winobj){
		if(cwin==frame->current_cwin)
			tabcolor=&(grdata->act_tab_sel_colors);
		else
			tabcolor=&(grdata->act_tab_colors);
	}else{
		if(cwin==frame->current_cwin)
			tabcolor=&(grdata->tab_sel_colors);
		else
			tabcolor=&(grdata->tab_colors);
	}
	
	XSetWindowBackground(wglobal.dpy, win, tabcolor->pixels[WCG_PIX_BG]);
	XClearWindow(wglobal.dpy, win);
	
	draw_tab(grdata, win, tabcolor, 0, 0, frame->tab_w, frame->bar_h, FALSE,
			 cwin->label, cwin->label_width);
}




/* */


void draw_frame_frame(const WFrame *frame, bool complete)
{
	WGRData *grdata=GRDATA;
	WColorGroup *colors;
#ifdef CF_WANT_TRANSPARENT_TERMS
	int fw, off, w, h;
#endif
	
	if((WWinObj*)frame==wglobal.current_winobj)
		colors=&(grdata->act_base_colors);
	else
		colors=&(grdata->base_colors);
	
#ifndef CF_WANT_TRANSPARENT_TERMS
	if(complete){
		XSetWindowBackground(wglobal.dpy, frame->frame_win,
							 colors->pixels[WCG_PIX_BG]);
		XClearWindow(wglobal.dpy, frame->frame_win);
	}
#else
	off=CF_BEVEL_WIDTH;
	fw=CF_BORDER_WIDTH-CF_BEVEL_WIDTH*2;
	w=FRAME_W(frame)-CF_BEVEL_WIDTH*2;
	h=FRAME_H(frame)-CF_BEVEL_WIDTH*2;

	XSetForeground(wglobal.dpy, grdata->gc, colors->pixels[WCG_PIX_BG]);
	
	/* top */
	XFillRectangle(wglobal.dpy, frame->frame_win, grdata->gc,
				   off, off, w, fw);
	/* bottom */
	XFillRectangle(wglobal.dpy, frame->frame_win, grdata->gc,
				   off, off+h-fw, w, fw);
	/* left */
	XFillRectangle(wglobal.dpy, frame->frame_win, grdata->gc,
				   off, off+fw, fw, h-fw*2);
	/* right */
	XFillRectangle(wglobal.dpy, frame->frame_win, grdata->gc,
				   off+w-fw, off+fw, fw, h-fw*2);

	/* center */
	XFillRectangle(wglobal.dpy, frame->frame_win, grdata->gc,
				   CF_BORDER_WIDTH, CF_BORDER_WIDTH,
				   FRAME_W(frame)-CF_BORDER_WIDTH*2,
				   FRAME_H(frame)-CF_BORDER_WIDTH*2);
#endif
	draw_border(grdata, frame->frame_win, colors, 0, 0,
				FRAME_W(frame), FRAME_H(frame));
}


void draw_frame_bar(const WFrame *frame, bool complete)
{
	WColorGroup *selcolor, *tabcolor, *colors;
	WClientWin *cwin;
	WGRData *grdata=GRDATA;
	int x=0;

	if(WFRAME_IS_NO_BAR(frame))
		return;
	
	if((WWinObj*)frame==wglobal.current_winobj){
		selcolor=&(grdata->act_tab_sel_colors);
		colors=tabcolor=&(grdata->act_tab_colors);
	}else{
		selcolor=&(grdata->tab_sel_colors);
		colors=tabcolor=&(grdata->tab_colors);
	}
	
	if(complete){
		XSetWindowBackground(wglobal.dpy, frame->bar_win,
							 tabcolor->pixels[WCG_PIX_BG]);
		XClearWindow(wglobal.dpy, frame->bar_win);
	}
	
	for(cwin=first_clientwin((WThing*)frame);
		cwin!=NULL;
		cwin=next_clientwin(cwin)){
		
		if(cwin==frame->current_cwin)
			colors=selcolor;
		else
			colors=tabcolor;
		
		draw_tab(grdata, frame->bar_win, colors,
				 x, 0, frame->tab_w, frame->bar_h, TRUE,
				 cwin->label, cwin->label_width);
		
		if(cwin->flags&CWIN_TAGGED){
			XSetForeground(wglobal.dpy, grdata->gc,
						   colors->pixels[WCG_PIX_FG]);
						   
			XDrawRectangle(wglobal.dpy, frame->bar_win, grdata->gc,
						   x+CF_BEVEL_WIDTH, CF_BEVEL_WIDTH,
						   frame->tab_w-1-2*CF_BEVEL_WIDTH,
						   frame->bar_h-1-2*CF_BEVEL_WIDTH);
			XDrawRectangle(wglobal.dpy, frame->bar_win, grdata->gc,
						   x+CF_BEVEL_WIDTH+1, CF_BEVEL_WIDTH+1,
						   frame->tab_w-3-2*CF_BEVEL_WIDTH,
						   frame->bar_h-3-2*CF_BEVEL_WIDTH);
		}
		
		if(cwin->flags&CWIN_DRAG){
			XFillRectangle(wglobal.dpy, frame->bar_win, grdata->stipple_gc,
						   x, 0, frame->tab_w, frame->bar_h);
		}
		
		x+=frame->tab_w;
	}

	if(WWINOBJ_IS_STICKY(frame)){
		XSetForeground(wglobal.dpy, grdata->copy_gc,
					   colors->pixels[WCG_PIX_FG]);
		copy_masked(grdata, grdata->stick_pixmap, frame->bar_win,
					0, 0, grdata->stick_pixmap_w, grdata->stick_pixmap_h,
					frame->bar_w-grdata->stick_pixmap_w-CF_BEVEL_WIDTH,
					CF_BEVEL_WIDTH);
	}
}


/* */


static int menu_entry_y(const WMenu *menu, int entry)
{
	int y=CF_MENU_V_SPACE;
	
	y+=menu->title_height;
	y+=entry*menu->entry_height;
	
	return y;
}


static void draw_menu_entrytext(const WMenu *menu, const WGRData *grdata,
								int y, const WMenuEnt *ent)
{
	XDrawString(wglobal.dpy, menu->menu_win, grdata->menu_gc,
				CF_MENUENT_H_SPACE,
				y+CF_MENUENT_V_SPACE+FONT_BASELINE(grdata->menu_font),
				ent->name, strlen(ent->name));
		
	if(ent->flags&WMENUENT_SUBMENU)
		XDrawString(wglobal.dpy, menu->menu_win, grdata->menu_gc,
					menu->w-grdata->submenu_ind_w-CF_MENUENT_H_SPACE,
					y+CF_MENUENT_V_SPACE+FONT_BASELINE(grdata->menu_font),
					"->", 2);
}


static void do_draw_menu_selection(const WMenu *menu, int y,
								   const WColorGroup *colors)
{
	draw_box(menu->menu_win, GRDATA->gc, colors, CF_BEVEL_WIDTH, y,
			 menu->w-2*CF_BEVEL_WIDTH, menu->entry_height, TRUE);
	XSetForeground(wglobal.dpy, GRDATA->menu_gc, colors->pixels[WCG_PIX_FG]);
	draw_menu_entrytext(menu, GRDATA, y,
						&(menu->data->entries[menu->selected]));
}


static void do_erase_menu_selection(const WMenu *menu, int y,
									const WColorGroup *colors)
{
	WGRData *grdata=GRDATA;
	
	XSetForeground(wglobal.dpy, grdata->gc, colors->pixels[WCG_PIX_BG]);
	XFillRectangle(wglobal.dpy, menu->menu_win, grdata->gc,
				   CF_BEVEL_WIDTH, y,
				   menu->w-2*CF_BEVEL_WIDTH,
				   menu->entry_height);
	XSetForeground(wglobal.dpy, GRDATA->menu_gc, colors->pixels[WCG_PIX_FG]);
	draw_menu_entrytext(menu, GRDATA, y,
						&(menu->data->entries[menu->selected]));
}


void draw_menu_selection(const WMenu *menu)
{
	WColorGroup *colors;

	if(menu->selected<0)
		return;
	
	colors=((WWinObj*)menu==wglobal.current_winobj ?
			&(GRDATA->act_sel_colors) :
			&(GRDATA->sel_colors));
	
	do_draw_menu_selection(menu, menu_entry_y(menu, menu->selected), colors);
}


void erase_menu_selection(const WMenu *menu)
{
	WColorGroup *colors;
	
	if(menu->selected<0)
		return;
	
	colors=((WWinObj*)menu==wglobal.current_winobj ?
			&(GRDATA->act_base_colors) :
			&(GRDATA->base_colors));
	
	do_erase_menu_selection(menu, menu_entry_y(menu, menu->selected), colors);
}


void draw_menu(const WMenu *menu, bool complete)
{
	WGRData *grdata=GRDATA;
	Window win=menu->menu_win;
	const WMenuData *data=menu->data;
	const WColorGroup *colors, *tcolors;
	int y=0, i, incr;
	WMenuEnt *ent;

	if((WWinObj*)menu==wglobal.current_winobj){
		colors=&(grdata->act_base_colors);
		tcolors=&(grdata->act_tab_sel_colors);
	}else{
		colors=&(grdata->base_colors);
		tcolors=&(grdata->tab_sel_colors);
	}
	
	if(complete){
		XSetWindowBackground(wglobal.dpy, menu->menu_win,
							 colors->pixels[WCG_PIX_BG]);
		XClearWindow(wglobal.dpy, win);
	}
	
	if(!(menu->flags&WMENU_NOTITLE)){
		y=menu->title_height;
		
		draw_box(win, GRDATA->gc, tcolors, 0, 0, menu->w, y, TRUE);

		if(menu->flags&WMENU_KEEP){
			XSetForeground(wglobal.dpy, grdata->copy_gc,
						   tcolors->pixels[WCG_PIX_FG]);
			copy_masked(grdata, grdata->stick_pixmap, win,
						0, 0, grdata->stick_pixmap_w, grdata->stick_pixmap_h,
						menu->w-grdata->stick_pixmap_w-CF_BEVEL_WIDTH,
						CF_BEVEL_WIDTH);
		}

		XSetForeground(wglobal.dpy, grdata->gc, tcolors->pixels[WCG_PIX_FG]);
		XDrawString(wglobal.dpy, win, grdata->gc, CF_MENUTITLE_H_SPACE,
					CF_MENUTITLE_V_SPACE+FONT_BASELINE(grdata->font),
					data->title, strlen(data->title));

	}

	incr=menu->entry_height;

	draw_box(win, GRDATA->gc, colors, 0, y, menu->w, menu->h-y, complete);

	XSetForeground(wglobal.dpy, grdata->menu_gc, colors->pixels[WCG_PIX_FG]);

	y+=CF_MENU_V_SPACE;
	
	for(i=0, ent=data->entries; i<data->nentries; i++, y+=incr, ent++){
		if(i==menu->selected)
			continue;
		draw_menu_entrytext(menu, grdata, y, ent);
	}
	
	draw_menu_selection(menu);
}


void draw_frame(const WFrame *frame, bool complete)
{
	draw_frame_frame(frame, complete);
	draw_frame_bar(frame, complete);
}


void draw_dock(const WDock *dock, bool complete)
{
	if(complete)
		XClearWindow(wglobal.dpy, dock->win);

	draw_box(dock->win, GRDATA->gc, &(GRDATA->base_colors), 0, 0,
			 dock->w, dock->h, FALSE);
}
