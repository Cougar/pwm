/*
 * pwm/event.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "common.h"
#include "event.h"
#include "clientwin.h"
#include "screen.h"
#include "frame.h"
#include "draw.h"
#include "property.h"
#include "pointer.h"
#include "key.h"
#include "focus.h"
#include "menu.h"
#include "cursor.h"
#include "signal.h"
#include "winlist.h"
#include "mwmhints.h"


static void handle_expose(const XExposeEvent *ev);
static void handle_map_request(const XMapRequestEvent *ev);
static void handle_configure_request(XConfigureRequestEvent *ev);
static void handle_enter_window(XEvent *ev);
static void handle_unmap_notify(const XUnmapEvent *ev);
static void handle_destroy_notify(const XDestroyWindowEvent *ev);
static void handle_client_message(const XClientMessageEvent *ev);
static void handle_focus_in(const XFocusChangeEvent *ev);
static void handle_focus_out(const XFocusChangeEvent *ev);
static void handle_property(const XPropertyEvent *ev);
static void handle_colormap_notify(const XColormapEvent *ev);
static void pointer_handler(XEvent *ev);
static void keyboard_handler();


/* */


void get_event(XEvent *ev)
{
	fd_set rfds;
	
	while(1){
		check_signals();
	
		/* Events waiting in client-side buffer? */
		if(QLength(wglobal.dpy)>0){
			XNextEvent(wglobal.dpy, ev);
			return;
		}
		
		XFlush(wglobal.dpy);

		FD_ZERO(&rfds);
		FD_SET(wglobal.conn, &rfds);

		if(select(wglobal.conn+1, &rfds, NULL, NULL, NULL)>0){
			XNextEvent(wglobal.dpy, ev);
			return;
		}
	}
}


void get_event_mask(XEvent *ev, long mask)
{
	fd_set rfds;
	bool found=FALSE;
	
	while(1){
		check_signals();
		
		while(XCheckMaskEvent(wglobal.dpy, mask, ev)){
			if(ev->type!=MotionNotify)
				return;
			found=TRUE;
		}

		if(found)
			return;
		
		/* Events waiting in kernel-side buffer? */
		FD_ZERO(&rfds);
		FD_SET(wglobal.conn, &rfds);

		select(wglobal.conn+1, &rfds, NULL, NULL, NULL);
	}
}


#define CASE_EVENT(X) case X: \
/*	fprintf(stderr, "[%#lx] %s\n", ev->xany.window, #X);*/


void handle_event(XEvent *ev)
{
	switch(ev->type){
	CASE_EVENT(MapRequest)
		handle_map_request(&(ev->xmaprequest));
		break;
	CASE_EVENT(ConfigureRequest)
		handle_configure_request(&(ev->xconfigurerequest));
		break;
	CASE_EVENT(UnmapNotify)
		handle_unmap_notify(&(ev->xunmap));
		break;
	CASE_EVENT(DestroyNotify)
		handle_destroy_notify(&(ev->xdestroywindow));
		break;
	CASE_EVENT(ClientMessage)
		handle_client_message(&(ev->xclient));
		break;
	CASE_EVENT(PropertyNotify)
		handle_property(&(ev->xproperty));
		break;
	CASE_EVENT(FocusIn)
		handle_focus_in(&(ev->xfocus));
		break;
	CASE_EVENT(FocusOut)
		handle_focus_out(&(ev->xfocus));
		break;
	CASE_EVENT(EnterNotify)
		handle_enter_window(ev);
		break;
	CASE_EVENT(Expose)		
		handle_expose(&(ev->xexpose));
		break;
	CASE_EVENT(KeyPress)
		handle_keypress(&(ev->xkey));
		break;
	CASE_EVENT(ButtonPress)
		pointer_handler(ev);
		break;
	CASE_EVENT(ColormapNotify)
		handle_colormap_notify(&(ev->xcolormap));
		break;
#if 0
	CASE_EVENT(ButtonRelease) break;
	CASE_EVENT(MotionNotify) break;
	CASE_EVENT(SelectionClear) break;
	CASE_EVENT(SelectionNotify) break;
	CASE_EVENT(SelectionRequest) break;
	CASE_EVENT(MapNotify) break;
	CASE_EVENT(ConfigureNotify) break;
	CASE_EVENT(MappingNotify) break;
	CASE_EVENT(LeaveNotify) break;
	CASE_EVENT(CreateNotify) break;
	CASE_EVENT(ReparentNotify) break;
#endif		
	default:
		break;
	}
}


void mainloop()
{
	XEvent ev;
	
	for(;;){
		get_event(&ev);
		handle_event(&ev);
		
		if(wglobal.input_mode==INPUT_MOVERES)
			keyboard_handler();
		
		XSync(wglobal.dpy, False);
			
		if(wglobal.focus_next!=NULL){
			while(XCheckMaskEvent(wglobal.dpy,
								  EnterWindowMask|FocusChangeMask, &ev))
				/* nothing */;
			
			do_set_focus(wglobal.focus_next);
			wglobal.focus_next=NULL;
		}
	}
}


/* */


static void handle_expose(const XExposeEvent *ev)
{
	WWinObj *winobj;
	WFrame *frame;
	XEvent tmp;
	
	while(XCheckWindowEvent(wglobal.dpy, ev->window, ExposureMask, &tmp))
		/* nothing */;

	winobj=(WWinObj*)find_thing_t(ev->window, WTHING_WINOBJ);

	if(winobj==NULL){
		if(ev->window==SCREEN->tabdrag_win &&  wglobal.grab_holder!=NULL &&
		   WTHING_IS(wglobal.grab_holder, WTHING_CLIENTWIN)){
			draw_tabdrag((WClientWin*)wglobal.grab_holder);
		}
		return;
	}

	if(WTHING_IS(winobj, WTHING_FRAME)){
		frame=(WFrame*)winobj;
		if(ev->window==frame->frame_win)
			draw_frame_frame(frame, FALSE);
		else if(ev->window==frame->bar_win)
			draw_frame_bar(frame, FALSE);
	}else if(WTHING_IS(winobj, WTHING_MENU)){
		draw_menu((WMenu*)winobj, FALSE);
	}else if(WTHING_IS(winobj, WTHING_DOCK)){
		draw_dock((WDock*)winobj, FALSE);
	}
}


static void handle_map_request(const XMapRequestEvent *ev)
{
	WThing *thing;
	
	thing=find_thing(ev->window);
	
	if(thing==NULL){
		if(ev->parent==SCREEN->root)
			manage_clientwin(ev->window, 0);
		return;
	}

#if 0
	if(WTHING_IS(thing, WTHING_CLIENTWIN)){
		switch(((WClientWin*)thing)->state){
		case NormalState:
			warn("Client in NormalState trying to map");
			break;
		case IconicState:
			/* Just ignore it. I don't want those freaking windows
			 * to pop up around my screen when I've iconified them. */
			warn("Ignoring de-iconify request");
			break;
		case WithdrawnState:
			break;
		}
	}
#endif
}


#ifndef CF_NO_WILD_WINDOWS
static void move_wildwin(WClientWin *cwin, int x, int y)
{
	WFrame *frame;
	
/*	if(!CWIN_IS_WILD(cwin))
		return;*/
	
	if(!CWIN_HAS_FRAME(cwin))
		return;
	
	frame=CWIN_FRAME(cwin);
	
	y-=frame->bar_h-frame->frame_iy;
	x-=frame->frame_ix;
	set_frame_pos(frame, x, y);
}
#endif


static void handle_configure_request(XConfigureRequestEvent *ev)
{
	WClientWin *cwin;
	XWindowChanges wc;
	
	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL){
		wc.border_width=ev->border_width;
		wc.sibling=None;
		wc.stack_mode=ev->detail;
		wc.x=ev->x;
		wc.y=ev->y;
		wc.width=ev->width;
		wc.height=ev->height;
		XConfigureWindow(wglobal.dpy, ev->window, ev->value_mask, &wc);
		return;
	}
	
	if((ev->value_mask&(CWWidth|CWHeight))!=0)
		set_clientwin_size(cwin, ev->width, ev->height);
	
#ifndef CF_NO_WILD_WINDOWS
	if((ev->value_mask&(CWX|CWY))!=0)
		move_wildwin(cwin, ev->x, ev->y);
#endif
}
	   

static void handle_enter_window(XEvent *ev)
{
	XEnterWindowEvent *eev=&(ev->xcrossing);
	WThing *thing=NULL;

	if(wglobal.input_mode==INPUT_CTXMENU)
		return;
	
#if 0
	/* Seems to miss some events this way */
	XEvent tmp;
	
	while(XCheckMaskEvent(wglobal.dpy, EnterWindowMask|FocusChangeMask, &tmp)){
		if(tmp.type==FocusOut)
			continue;
		*ev=tmp;
	}
		
	if(ev->type==FocusIn){
		handle_focus_in(&(ev->xfocus));
		return;
	}
#else
	while(XCheckMaskEvent(wglobal.dpy, EnterWindowMask, ev))
		/* nothing */;
#endif
	
	if(eev->window==eev->root){
		if(!eev->same_screen)
			thing=(WThing*)wglobal.current_winobj;
		else
			return;
	}else{
		thing=find_thing(eev->window);

		if(thing==NULL)
			return;
	
		if(WTHING_IS(thing, WTHING_CLIENTWIN)){
			if(CWIN_FRAME((WClientWin*)thing)==NULL)
				return;
		}
	}
	
	do_set_focus(thing);
}


static void handle_unmap_notify(const XUnmapEvent *ev)
{
	WThing *thing;

	/* We are not interested in SubstructureNotify -unmaps. */
	if(ev->event!=ev->window)
		return;

	thing=find_thing(ev->window);
	
	if(thing==NULL)
		return;

	if(WTHING_IS(thing, WTHING_CLIENTWIN))
		unmap_clientwin((WClientWin*)thing);
}


static void handle_destroy_notify(const XDestroyWindowEvent *ev)
{
	WClientWin *cwin;

	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL)
		return;
	
	destroy_clientwin(cwin);
}


static void handle_client_message(const XClientMessageEvent *ev)
{
	WClientWin *cwin;
					   
	if(ev->message_type!=wglobal.atom_wm_change_state)
		return;
	
	cwin=find_clientwin(ev->window);
	
	if(cwin!=NULL && ev->format==32 && ev->data.l[0]==IconicState){
		if(cwin->state==NormalState)
			iconify_clientwin(cwin);
	}
}


/* */


static bool current_cwin(WClientWin *cwin)
{
	if(cwin->t_parent!=(WThing*)wglobal.current_winobj)
		return FALSE;
	
	return (((WFrame*)wglobal.current_winobj)->current_cwin==cwin);
}


static void handle_cwin_cmap(WClientWin *cwin, const XColormapEvent *ev)
{
	int i;
	
	if(ev->window==cwin->client_win){
		cwin->cmap=ev->colormap;
		if(current_cwin(cwin))
			install_cwin_cmap(cwin);
	}else{
		for(i=0; i<cwin->n_cmapwins; i++){
			if(cwin->cmapwins[i]!=ev->window)
				continue;
			cwin->cmaps[i]=ev->colormap;
			if(current_cwin(cwin))
				install_cwin_cmap(cwin);
			break;
		}
	}
}


static void handle_colormap_notify(const XColormapEvent *ev)
{
	WClientWin *cwin;
	
	if(!ev->new)
		return;

	cwin=find_clientwin(ev->window);

	if(cwin!=NULL){
		handle_cwin_cmap(cwin, ev);
	}else{
		for(cwin=SCREEN->clientwin_list; cwin!=NULL; cwin=cwin->s_cwin_next)
			handle_cwin_cmap(cwin, ev);
	}
}


/* */


static void activate(WWinObj *obj)
{
	wglobal.current_winobj=obj;
	if(WTHING_IS(obj, WTHING_FRAME)){
		activate_frame((WFrame*)obj);
	}else if(WTHING_IS(obj, WTHING_MENU)){
		activate_menu((WMenu*)obj);
	}
}


static void deactivate(WWinObj *obj)
{
	wglobal.current_winobj=NULL;
	if(WTHING_IS(obj, WTHING_FRAME)){
		deactivate_frame((WFrame*)obj);
	}else if(WTHING_IS(obj, WTHING_MENU)){
		deactivate_menu((WMenu*)obj);
	}
}


static void handle_focus_in(const XFocusChangeEvent *ev)
{
	WThing *thing;
	WWinObj *winobj;
	
	thing=find_thing(ev->window);
	
	if(thing==NULL){
		if(ev->window==SCREEN->root)
			thing=(WThing*)SCREEN;
		else		
			return;
	}
	
	if(WTHING_IS(thing, WTHING_CLIENTWIN))
		install_cwin_cmap((WClientWin*)thing);
	else
		install_cmap(None);

	winobj=winobj_of(thing);
	
	if(wglobal.current_winobj==winobj)
		return;

	if(wglobal.current_winobj!=NULL){
		wglobal.previous_winobj=wglobal.current_winobj;
		deactivate(wglobal.current_winobj);
	}

	if(winobj!=NULL)
		activate(winobj);
}


static void handle_focus_out(const XFocusChangeEvent *ev)
{
#if 0
	WWinObj *winobj;
	
	if((winobj=find_winobj_of(ev->window))==NULL)
		return;
	
	deactivate(winobj);
#endif
}


static void handle_property(const XPropertyEvent *ev)
{
	WClientWin *cwin;
	WFrame *frame;
	bool need_recalc=FALSE;
	int flags;
	
	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL)
		return;
	
	frame=CWIN_FRAME(cwin);
	
	switch(ev->atom){
	case XA_WM_NORMAL_HINTS:
		get_clientwin_size_hints(cwin);
		break;
	
	case XA_WM_NAME:
		if(cwin->name!=NULL)
			XFree((void*)cwin->name);
		cwin->name=get_string_property(cwin->client_win, XA_WM_NAME, NULL);
		need_recalc=TRUE;
		break;
		
	case XA_WM_ICON_NAME:
		if(cwin->icon_name!=NULL)
			XFree((void*)cwin->icon_name);
		cwin->icon_name=get_string_property(cwin->client_win,
											XA_WM_ICON_NAME, NULL);
		if(cwin->name==NULL)
			need_recalc=TRUE;
		break;

	case XA_WM_TRANSIENT_FOR:
		warn("Changes in WM_TRANSIENT_FOR property are unsupported.");
		return;
	default:
		if(ev->atom==wglobal.atom_mwm_hints){
			get_mwm_hints(ev->window, &flags);
			cwin->flags=(cwin->flags&~CWIN_WILD)|flags;
			if(frame!=NULL && frame->cwin_count==1)
				frame_set_decor(frame, flags&CWIN_WILD ? WFRAME_NO_DECOR : 0);
		}
		return;
	}
	
	if(need_recalc){
		clientwin_unuse_label(cwin);
		clientwin_use_label(cwin);
		update_winlist();
	}
}


void grab_kb_ptr()
{
	XSelectInput(wglobal.dpy, SCREEN->root, ROOT_MASK&~FocusChangeMask);
	XGrabPointer(wglobal.dpy, SCREEN->root, True, GRAB_POINTER_MASK,
				 GrabModeAsync, GrabModeAsync, None,
				 x_cursor(CURSOR_DEFAULT), CurrentTime);
	XGrabKeyboard(wglobal.dpy, SCREEN->root, False, GrabModeAsync,
				  GrabModeAsync, CurrentTime);
	XSelectInput(wglobal.dpy, SCREEN->root, ROOT_MASK);
}


void ungrab_kb_ptr()
{
	XUngrabKeyboard(wglobal.dpy, CurrentTime);
	XUngrabPointer(wglobal.dpy, CurrentTime);
	wglobal.grab_holder=NULL;
	wglobal.input_mode=INPUT_NORMAL;
}


void pointer_handler(XEvent *ev)
{
	grab_kb_ptr();
	
	handle_button_press(&(ev->xbutton));

	while(1){
		XFlush(wglobal.dpy);
		get_event_mask(ev, GRAB_POINTER_MASK|ExposureMask|
					   KeyPressMask|KeyReleaseMask|
					   EnterWindowMask|FocusChangeMask);
		
		switch(ev->type){
		CASE_EVENT(ButtonRelease)
			if(handle_button_release(&(ev->xbutton)))
				break;
			continue;
		CASE_EVENT(MotionNotify)
			handle_pointer_motion(&(ev->xmotion));
			continue;
		CASE_EVENT(Expose)		
			handle_expose(&(ev->xexpose));
			continue;
		default:
			continue;
		}
		break;
	}

	ungrab_kb_ptr();
}


void keyboard_handler()
{
	XEvent ev;
	
	grab_kb_ptr();

	while(1){
		XFlush(wglobal.dpy);
		get_event_mask(&ev, GRAB_POINTER_MASK|ExposureMask|KeyPressMask);
		
		switch(ev.type){
		case Expose:
			handle_expose(&(ev.xexpose));
			break;
		case KeyPress:
			handle_keypress(&(ev.xkey));
			break;
		}
		if(wglobal.input_mode!=INPUT_MOVERES)
			break;
	}
	
	ungrab_kb_ptr();
}
