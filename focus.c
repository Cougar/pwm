/*
 * pwm/focus.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include "common.h"
#include "focus.h"
#include "screen.h"
#include "clientwin.h"
#include "frame.h"
#include "menu.h"
#include "workspace.h"


void do_set_focus(WThing *thing)
{
	WFrame *frame;
	WClientWin *cwin=NULL;
	Window win;

	if(WTHING_IS_UNFOCUSABLE(thing))
		return;
	
	if(WTHING_IS(thing, WTHING_WINOBJ)){
		if(!on_current_workspace((WWinObj*)thing)){
			if(wglobal.current_winobj!=NULL)
				return;
			thing=(WThing*)SCREEN;
		}
	}
	
	if(WTHING_IS(thing, WTHING_FRAME)){
		frame=(WFrame*)thing;

		if(!WFRAME_IS_NORMAL(frame) || frame->current_cwin==NULL){
			/* The frame is in shade mode or missing client window.
			 * Just sink keyboard input events in the tab bar.
			 */
			win=frame->bar_win;
		}else{
			cwin=frame->current_cwin;
			win=cwin->client_win;
		}
	}else if(WTHING_IS(thing, WTHING_CLIENTWIN)){
		cwin=(WClientWin*)thing;
		win=cwin->client_win;
	}else if(WTHING_IS(thing, WTHING_MENU)){
		win=((WMenu*)thing)->menu_win;
	}else if(WTHING_IS(thing, WTHING_SCREEN)){
		win=SCREEN->root;
		/*if(SCREEN->current_cmap!=None)
			XInstallColormap(wglobal.dpy, SCREEN->default_cmap);*/
	}else{
		return;
	}

	XSetInputFocus(wglobal.dpy, win, RevertToParent, CurrentTime);
	
	if(cwin!=NULL && cwin->flags&CWIN_P_WM_TAKE_FOCUS)
		send_clientmsg(cwin->client_win, wglobal.atom_wm_take_focus);
}


void set_focus_weak(WThing *thing)
{
	if(wglobal.focus_next==NULL)
		do_set_focus(thing);
}


void set_focus(WThing *thing)
{
	wglobal.focus_next=thing;
}


static int sign(int i)
{
	return (i>0 ? 1 : i<0 ? -1 : 0);
}


static bool try_focus(WThing *next)
{
	if(WTHING_IS(next, WTHING_WINOBJ) &&
	   !WTHING_IS_UNFOCUSABLE(next) &&
	   on_current_workspace((WWinObj*)next)){
		do_set_focus(next);
		return TRUE;
	}
	
	return FALSE;
}


static WWinObj *do_circulate(int dir)
{
	WThing *tmp=(WThing*)wglobal.current_winobj;
	WThing *next;
	int sgn=sign(dir);
	
	if(tmp==NULL){
		tmp=SCREEN->t_children;
		if(tmp==NULL || try_focus(tmp))
			return (WWinObj*)tmp;
	}

	next=tmp;
	
	do{
		next=nth_thing(next, dir);
		dir=sgn;
		
		if(next==NULL || next==tmp)
			break;
		
		if(try_focus(next))
			return (WWinObj*)next;
	}while(1);
	
	return NULL;
}


WWinObj *circulate(int dir)
{
	return do_circulate(dir);
}


void circulateraise(int dir)
{
	WWinObj *obj=do_circulate(dir);
	
	if(obj!=NULL)
		raise_winobj(obj);
}


void goto_previous()
{
	WWinObj *obj=wglobal.previous_winobj;
	
	if(obj!=NULL){
		if(!on_current_workspace(obj))
			dodo_switch_workspace(workspace_of(obj));
		try_focus((WThing*)wglobal.previous_winobj);
	}
}

