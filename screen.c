/*
 * pwm/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <stdio.h>
#include <limits.h>
#include <X11/Xlib.h>
#if 0
#include <X11/Xmu/Error.h>
#endif
#include <X11/Xproto.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "common.h"
#include "screen.h"
#include "config.h"
#include "event.h"
#include "cursor.h"
#include "frame.h"
#include "draw.h"


/* */


static bool redirect_error=FALSE;
static bool ignore_badwindow=TRUE;


static int my_redirect_error_handler(Display *dpy, XErrorEvent *ev)
{
	redirect_error=TRUE;
	return 0;
}


static int my_error_handler(Display *dpy, XErrorEvent *ev)
{
	static char msg[128], request[64], num[32];
	
	/* Just ignore bad window and similar errors; makes the rest of
	 * the code simpler.
	 */
	if((ev->error_code==BadWindow ||
		(ev->error_code==BadMatch && ev->request_code==X_SetInputFocus) ||
		(ev->error_code==BadDrawable && ev->request_code==X_GetGeometry)) &&
	   ignore_badwindow)
		return 0;

#if 0
	XmuPrintDefaultErrorMessage(dpy, ev, stderr);
#else
	XGetErrorText(dpy, ev->error_code, msg, 128);
	sprintf(num, "%d", ev->request_code);
	XGetErrorDatabaseText(dpy, "XRequest", num, "", request, 64);

	if(request[0]=='\0')
		sprintf(request, "<unknown request>");

	if(ev->minor_code!=0){
		warn("[%d] %s (%d.%d) %#lx: %s", ev->serial, request,
			 ev->request_code, ev->minor_code, ev->resourceid,msg);
	}else{
		warn("[%d] %s (%d) %#lx: %s", ev->serial, request,
			 ev->request_code, ev->resourceid,msg);
	}
#endif

	kill(getpid(), SIGTRAP);
	
	return 0;
}


/* */


static bool do_alloc_color(Display *dpy, Colormap cmap, const char *name,
						   ulong *cret)
{
	XColor c;
	bool ret=FALSE;

	if(XParseColor(dpy, cmap, name, &c)){
		ret=XAllocColor(dpy, cmap, &c);
		*cret=c.pixel;
	}
	return ret;
}


bool alloc_color(const char *name, ulong *cret)
{
	return do_alloc_color(wglobal.dpy, SCREEN->default_cmap, name, cret);
}
	


static void init_color_group(WColorGroup *cg, int hl, int sh, int bg, int fg)
{
	cg->pixels[WCG_PIX_BG]=bg;
	cg->pixels[WCG_PIX_HL]=hl;
	cg->pixels[WCG_PIX_SH]=sh;
	cg->pixels[WCG_PIX_FG]=fg;
}

							 
/* */


static void scan_initial_windows()
{
	Window dummy_root, dummy_parent, *wins;
	uint nwins, i, j;
	XWMHints *hints;
	
	XQueryTree(wglobal.dpy, SCREEN->root,
			   &dummy_root, &dummy_parent, &wins, &nwins);
	
	for(i=0; i<nwins; i++){
		if(wins[i]==None)
			continue;
		hints=XGetWMHints(wglobal.dpy, wins[i]);
		if(hints!=NULL && hints->flags&IconWindowHint){
			for(j=0; j<nwins; j++){
				if(wins[j]==hints->icon_window){
					wins[j]=None;
					break;
				}
			}
		}
		if(hints!=NULL)
			XFree((void*)hints);

	}

	for(i=0; i<nwins; i++){
		if(wins[i]==None)
			continue;
		manage_clientwin(wins[i], MANAGE_RESPECT_POS|MANAGE_INITIAL);
	}
	
	XFree((void*)wins);
}


Window create_simple_window(int x, int y, int w, int h, ulong background)
{
	return XCreateSimpleWindow(wglobal.dpy, SCREEN->root, x, y, w, h,
							   0, BlackPixel(wglobal.dpy, SCREEN->xscr),
							   background);
}


static int chars_for_num(int d)
{
	int n=0;
	
	do{
		n++;
		d/=10;
	}while(d);

	return n;
}


static int max_width(XFontStruct *font, const char *str)
{
	int maxw=0, w;
	
	while(*str!='\0'){
		w=XTextWidth(font, str, 1);
		if(w>maxw)
			maxw=w;
		str++;
	}
	
	return maxw;
}


static void create_wm_windows()
{
	WScreen *scr=SCREEN;
	int maxl;
	int cw;
	
	/* Create move/resize position/size display window */
	maxl=3;
	maxl+=chars_for_num(scr->width);
	maxl+=chars_for_num(scr->height);
	
	cw=max_width(GRDATA->font, "0123456789x+"); 
	/*grdata->font->max_bounds.width;*/
	
	maxl*=cw;
	
	maxl+=CF_TAB_MAX_TEXT_X_OFF;
	
	scr->moveres_win_w=maxl;
	scr->moveres_win_h=GRDATA->bar_height;
	
	scr->moveres_win=
		create_simple_window(CF_MOVERES_WIN_X, CF_MOVERES_WIN_Y,
							 scr->moveres_win_w, scr->moveres_win_h,
							 GRDATA->tab_sel_colors.pixels[WCG_PIX_BG]);
	
	/* Create tab drag window */
	scr->tabdrag_win=
		create_simple_window(0, 0, 1, 1, BlackPixel(wglobal.dpy, scr->xscr));
	
	XSelectInput(wglobal.dpy, scr->tabdrag_win, ExposureMask);
}


/* */


bool preinit_screen(int xscr)
{
	Display *dpy=wglobal.dpy;
	ulong white, black;
	WGRData *grdata=GRDATA;
	WScreen *scr=SCREEN;

	WTHING_INIT(scr, WTHING_SCREEN);
	
	/* Initial settings */
	scr->clientwin_list=NULL;
	scr->n_clientwin=0;
	scr->n_workspaces=0;
	scr->current_workspace=0;
	
	scr->xscr=xscr;
	scr->root=RootWindow(dpy, xscr);
	scr->width=DisplayWidth(dpy, xscr);
	scr->height=DisplayHeight(dpy, xscr);
	scr->default_cmap=DefaultColormap(dpy, xscr);
	scr->opaque_move=100;
	
	/* Try to select input on the root window */
	redirect_error=FALSE;

	XSetErrorHandler(my_redirect_error_handler);
	XSelectInput(dpy, scr->root, ROOT_MASK);
	XSync(dpy, 0);
	XSetErrorHandler(my_error_handler);

	if(redirect_error){
		warn("Unable to redirect root window events for screen %d.", xscr);
		return FALSE;
	}

	/*
	 * graphics defaults init
	 */
	
	black=BlackPixel(wglobal.dpy, scr->xscr);
	white=WhitePixel(wglobal.dpy, scr->xscr);
	
											/* hl, sh, bg, fg */
	init_color_group(&(grdata->tab_sel_colors), white, white, white, black);
	init_color_group(&(grdata->tab_colors), black, black, white, black);
	init_color_group(&(grdata->base_colors), white, white, white, black);
	init_color_group(&(grdata->sel_colors), white, white, white, black);

	init_color_group(&(grdata->act_tab_sel_colors), white, white, black, white);
	init_color_group(&(grdata->act_tab_colors), black, black, black, white);
	init_color_group(&(grdata->act_base_colors), white, white, white, black);
	init_color_group(&(grdata->act_sel_colors), white, white, white, black);

	grdata->border_width=6;
	grdata->bevel_width=2;
	grdata->bar_min_width=10;
	grdata->bar_max_width_q=0.95;
	grdata->tab_min_width=10;

	grdata->autoraise_time=-1;
	
	return TRUE;
}


void postinit_screen()
{
	XGCValues gcv;
	ulong gcvmask;
	Pixmap stipple_pixmap;
	GC tmp_gc;
	Display *dpy=wglobal.dpy;
	ulong white, black;
	WGRData *grdata=GRDATA;
	WScreen *scr=SCREEN;

	/*
	 * graphics init
	 */
	
	black=BlackPixel(wglobal.dpy, scr->xscr);
	white=WhitePixel(wglobal.dpy, scr->xscr);

	/* font */
	if(grdata->font==NULL)
		grdata->font=load_font(dpy, CF_FALLBACK_FONT_NAME);
	
	if(grdata->menu_font==NULL)
		grdata->menu_font=load_font(dpy, CF_FALLBACK_FONT_NAME);

	grdata->bar_height=FONT_HEIGHT(grdata->font)+2*CF_TAB_TEXT_Y_OFF;
	grdata->submenu_ind_w=XTextWidth(grdata->menu_font, "->", 2);

	/* Create normal gc */
	gcv.line_style=LineSolid;
	gcv.line_width=1;
	gcv.join_style=JoinBevel;
	gcv.cap_style=CapButt;
	gcv.fill_style=FillSolid;
	gcv.font=grdata->font->fid;

	gcvmask=GCForeground|GCBackground|GCLineStyle|GCLineWidth|
			GCFillStyle|GCJoinStyle|GCCapStyle|GCFont;
	
	grdata->gc=XCreateGC(dpy, scr->root, gcvmask, &gcv);

	/* Create menu gc */
	gcv.font=grdata->menu_font->fid;
	grdata->menu_gc=XCreateGC(dpy, scr->root, gcvmask, &gcv);

	/* Create stipple pattern and stipple GC */
	stipple_pixmap=XCreatePixmap(dpy, scr->root, 2, 2, 1);
	gcv.foreground=1;
	tmp_gc=XCreateGC(dpy, stipple_pixmap, GCForeground, &gcv);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 1);
	XSetForeground(dpy, tmp_gc, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 1, 0);
	XDrawPoint(dpy, stipple_pixmap, tmp_gc, 0, 1);
	
	gcv.foreground=white;
	gcv.fill_style=FillStippled;
	gcv.stipple=stipple_pixmap;
	
	grdata->stipple_gc=XCreateGC(dpy, scr->root, gcvmask|GCStipple, &gcv);
	
	XFreePixmap(dpy, stipple_pixmap);
	
	/* Create stick pixmap */
	grdata->stick_pixmap_w=7;
	grdata->stick_pixmap_h=7;
	grdata->stick_pixmap=XCreatePixmap(dpy, scr->root, 7, 7, 1);
	
	XSetForeground(wglobal.dpy, tmp_gc, 0);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 0, 0, 7, 7);
	XSetForeground(wglobal.dpy, tmp_gc, 1);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 0, 2, 5, 2);
	XFillRectangle(wglobal.dpy, grdata->stick_pixmap, tmp_gc, 3, 4, 2, 3);
	
	XFreeGC(dpy, tmp_gc);

	/* Create copy gc */
	gcv.foreground=black;
	gcv.background=white;
	gcv.line_width=2;
	grdata->copy_gc=XCreateGC(dpy, scr->root,
							  GCLineWidth|GCForeground|GCBackground, &gcv);

	/* Create XOR gc (for resize) */
	gcv.subwindow_mode=IncludeInferiors;
	gcv.function=GXxor;
	gcv.foreground=ULONG_MAX; /*white;*/
	gcv.fill_style=FillSolid;
	gcvmask|=GCFunction|GCSubwindowMode|GCForeground;

	grdata->xor_gc=XCreateGC(dpy, scr->root, gcvmask, &gcv);

	/* */

	init_workspaces();
	scan_initial_windows();
	create_wm_windows();
	set_cursor(scr->root, CURSOR_DEFAULT);
}


/* */


void deinit_screen()
{
	destroy_subthings((WThing*)SCREEN);

	/* all client windows should be withdrawn when the frame is destroyed
	   though...*/
	while(SCREEN->clientwin_list!=NULL){
		warn("Still found managed client windows.");
		unmanage_clientwin(SCREEN->clientwin_list);
	}	
}
