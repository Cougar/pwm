/*
 * pwm/dock.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include "dock.h"
#include "screen.h"
#include "event.h"
#include "clientwin.h"
#include "draw.h"
#include "focus.h"
#include "binding.h"


#define DOCKWIN_W 64
#define DOCKWIN_H 64


static int create_x=0;
static int create_y=0;
static uint create_flags=0;


static bool do_create_dock(WDock *dock, int x, int y, int flags)
{
	Window win;
	
	flags|=WTHING_UNFOCUSABLE;
	
	dock->x=x;
	dock->y=y;
	dock->w=DOCKWIN_W;
	dock->h=DOCKWIN_H;
	
	win=create_simple_window(x, y, dock->w, dock->h,
							 GRDATA->base_colors.pixels[WCG_PIX_BG]);
	
	dock->flags|=flags;
	dock->win=win;
	dock->dockwin_count=0;
	dock->max_vis_dockwin=1;
	dock->dock_w=1;
	dock->dock_h=1;
	SCREEN->dock=dock;
	
	XSelectInput(wglobal.dpy, win, DOCK_MASK);
	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)dock);

	grab_bindings(win, ACTX_DOCKWIN|ACTX_GLOBAL);

	add_winobj((WWinObj*)dock, WORKSPACE_STICKY, LVL_KEEP_ON_TOP);
	
	return TRUE;
}


void set_dock_params(const char *geom, const uint flags)
{
	uint dummy;
	int flag;
	
	flag=XParseGeometry(geom, &create_x, &create_y, &dummy, &dummy);
	
	create_flags=flags;
	
	if(flag&XNegative)
		create_flags|=DOCK_RIGHT;
	if(flag&YNegative)
		create_flags|=DOCK_BOTTOM;
}

	
static WDock *create_dock(int x, int y, bool horiz)
{
	WDock *dock;
	
	dock=ALLOC(WDock);
	
	if(dock==NULL){
		warn_err();
		return NULL;
	}
	
	WTHING_INIT(dock, WTHING_DOCK);
	
	if(create_flags&DOCK_RIGHT)
		create_x=SCREEN->width+create_x-DOCKWIN_W;
	if(create_flags&DOCK_BOTTOM)
		create_y=SCREEN->height+create_y-DOCKWIN_H;
	
	if(!do_create_dock(dock, create_x, create_y, create_flags)){
		free(dock);
		return NULL;
	}
	
	return dock;
}


/* */


static void calc_dockwin_xy(WDock *dock, WClientWin *cwin,
							int nth, int *xret, int *yret)
{
	int h=dock->dock_h;
	
	*xret=(nth/h)*DOCKWIN_W;
	*yret=(nth%h)*DOCKWIN_H;
	
	if(cwin->client_w<DOCKWIN_W)
		*xret+=(DOCKWIN_W-cwin->client_w)/2;
	if(cwin->client_h<DOCKWIN_H)
		*yret+=(DOCKWIN_H-cwin->client_h)/2;
}


WClientWin* dockwin_at(WDock *dock, int x, int y)
{
	int n;
	
	x/=DOCKWIN_W;
	y/=DOCKWIN_H;
	
	n=x+y*dock->dock_w;
	
	return (WClientWin*)nth_subthing((WThing*)dock, n);
}


static void dock_resized(WDock *dock)
{
	int i=0;
	int x, y;
	WClientWin *cwin;

	dock->w=dock->dock_w*DOCKWIN_W;
	dock->h=dock->dock_h*DOCKWIN_H;

	XMoveResizeWindow(wglobal.dpy, dock->win,
					  dock->x, dock->y, dock->w, dock->h);
	
	dock->max_vis_dockwin=dock->dock_w*dock->dock_h;
	
	cwin=first_clientwin((WThing*)dock);
	
	while(cwin!=NULL){
		calc_dockwin_xy(dock, cwin, i, &x, &y);
		XMoveWindow(wglobal.dpy, cwin->client_win, x, y);
		/*clientwin_reconf_at(cwin, dock->x+x, dock->y+y);*/
		cwin=next_clientwin(cwin);
		i++;
	}
}


static void grow_dock(WDock *dock)
{
	if(dock->flags&DOCK_HORIZONTAL){
		dock->dock_w++;
		if(dock->flags&DOCK_RIGHT)
			dock->x-=DOCKWIN_W;
	}else{
		dock->dock_h++;
		if(dock->flags&DOCK_BOTTOM)
			dock->y-=DOCKWIN_H;
	}
	
	dock_resized(dock);
}


static void shrink_dock(WDock *dock)
{
	if(dock->flags&DOCK_HORIZONTAL){
		dock->dock_w--;
		if(dock->flags&DOCK_RIGHT)
			dock->x+=DOCKWIN_W;
	}else{
		dock->dock_h--;
		if(dock->flags&DOCK_BOTTOM)
			dock->y+=DOCKWIN_H;
	}

	dock_resized(dock);
}


static bool do_add_dockwin(WDock *dock, WClientWin *dw)
{
	WClientWin *dw2;
	
	dw->type=WTHING_DOCKWIN;
	dw->flags|=WTHING_UNFOCUSABLE;
	
	dw2=first_clientwin((WThing*)dock);
	while(dw2!=NULL){
		if(dw2->dockpos>dw->dockpos)
			break;
		dw2=next_clientwin(dw2);
	}
	
	if(dw2!=NULL)
		link_thing_before((WThing*)dw2, (WThing*)dw);
	else
		link_thing((WThing*)dock, (WThing*)dw);

	if(dw->client_w>DOCKWIN_W || dw->client_h>DOCKWIN_H){
		XResizeWindow(wglobal.dpy, dw->client_win, DOCKWIN_W, DOCKWIN_H);
		dw->client_w=DOCKWIN_W;
		dw->client_h=DOCKWIN_H;
	}
	
	XSelectInput(wglobal.dpy, dw->client_win, dw->event_mask&~StructureNotifyMask);
	XReparentWindow(wglobal.dpy, dw->client_win, dock->win, 0, 0);
	XSelectInput(wglobal.dpy, dw->client_win, dw->event_mask);
	XMapWindow(wglobal.dpy, dw->client_win);

	dock->dockwin_count++;

	if(dock->max_vis_dockwin<dock->dockwin_count)
		grow_dock(dock);
	else
		dock_resized(dock);
	
	return TRUE;
}


bool add_dockwin(WClientWin *dw)
{
	WDock *dock=SCREEN->dock;
	
	if(dock==NULL)
		dock=create_dock(0, 0, FALSE);
	
	if(dock==NULL || !do_add_dockwin(dock, dw))
		return FALSE;
	
	if(dock->dockwin_count==1)
		map_winobj((WWinObj*)dock);
	
	return TRUE;
}


/* */


static void do_remove_dockwin(WDock *dock, WClientWin *dw, bool subdest)
{
	WClientWin *p;
	int num=0;
	int x, y;
	
	p=first_clientwin((WThing*)dock);
	while(p!=NULL){
		if(p==dw)
			break;
		num++;
		p=next_clientwin(p);
	}
	
	if(p==NULL)
		return;

	x=dock->x;
	y=dock->y;
	
	XSelectInput(wglobal.dpy, dw->client_win,
				 dw->event_mask&~StructureNotifyMask);
	XReparentWindow(wglobal.dpy, dw->client_win, SCREEN->root, x, y);
	XSelectInput(wglobal.dpy, dw->client_win, dw->event_mask);
	
	unlink_thing((WThing*)dw);
	
	dock->dockwin_count--;

	if(dock->dockwin_count==0 || subdest)
		return;
	
	shrink_dock(dock);
}


void remove_dockwin(WClientWin *dw)
{
	WDock *dock=SCREEN->dock;
	
	if(dock==NULL)
		return;
	
	do_remove_dockwin(dock, dw, dock->flags&WTHING_SUBDEST);
	
	if(dock->dockwin_count==0 && !(dock->flags&WTHING_SUBDEST))
		unmap_winobj((WWinObj*)dock);
}


void destroy_dock(WDock *dock)
{
	unlink_winobj_d((WWinObj*)dock);

	SCREEN->dock=NULL;

	XDeleteContext(wglobal.dpy, dock->win, wglobal.win_context);
	XDestroyWindow(wglobal.dpy, dock->win);
	
	free_thing((WThing*)dock);
}


void dock_toggle_hide()
{
	WDock *dock=SCREEN->dock;
	
	if(dock==NULL)
		return;

	dock->flags^=WWINOBJ_HIDDEN;
	
	if(!WWINOBJ_IS_HIDDEN(dock)){
		if(dock->dockwin_count!=0)
			map_winobj((WWinObj*)dock);
	}else{
		unmap_winobj((WWinObj*)dock);
	}
}
		   

void dock_toggle_dir()
{
	WDock *dock=SCREEN->dock;
	int tmp;
	
	if(dock==NULL)
		return;
	
	dock->flags^=DOCK_HORIZONTAL;
	
	tmp=dock->dock_w;
	dock->dock_w=dock->dock_h;
	dock->dock_h=tmp;
	
	if(dock->flags&DOCK_RIGHT)
		dock->x=SCREEN->width-dock->h;

	if(dock->flags&DOCK_BOTTOM)
		dock->y=SCREEN->height-dock->w;
	
	dock_resized(dock);
}


void set_dock_pos(WDock *dock, int x, int y)
{
	WClientWin *cwin;
	int i=0;
	
	XMoveWindow(wglobal.dpy, dock->win, x, y);
	dock->x=x;
	dock->y=y;
	
	dock->flags&=~(DOCK_RIGHT|DOCK_BOTTOM);
	
	if(dock->x+dock->w==SCREEN->width)
		dock->flags|=DOCK_RIGHT;
	if(dock->y+dock->h==SCREEN->height)
		dock->flags|=DOCK_BOTTOM;
	
	cwin=first_clientwin((WThing*)dock);
	
	while(cwin!=NULL){
		calc_dockwin_xy(dock, cwin, i, &x, &y);
		clientwin_reconf_at(cwin, dock->x+x, dock->y+y);
		cwin=next_clientwin(cwin);
		i++;
	}
}

