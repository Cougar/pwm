/*
 * pwm/workspace.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include <X11/Xmd.h>

#include "common.h"
#include "screen.h"
#include "workspace.h"
#include "property.h"
#include "winlist.h"
#include "frame.h"
#include "focus.h"


bool visible_workspace(int ws)
{
	return (ws==SCREEN->current_workspace || ws==WORKSPACE_STICKY ||
			ws==WORKSPACE_CURRENT);
}


bool on_current_workspace(WWinObj *obj)
{
	return visible_workspace(obj->workspace);

}


int workspace_of(WWinObj *obj)
{
	if(!winobj_is_visible(obj))
		return WORKSPACE_UNKNOWN;
	
	return obj->workspace;
}

	
/* */


void dodo_switch_workspace(int num)
{
	
	WScreen *scr=SCREEN;
	WThing *thing, *next;
	WWinObj *obj;
	int old;
	
	old=scr->current_workspace;
	
	if(old==num)
		return;

	scr->current_workspace=num;

	for(thing=scr->t_children; thing!=NULL; thing=next){
		next=thing->t_next;
		
		if(!WTHING_IS(thing, WTHING_WINOBJ))
			continue;

		obj=(WWinObj*)thing;
		
		if(WWINOBJ_IS_STICKY(obj))
			continue;
		
		if(on_current_workspace(obj))
			do_map_winobj(obj);
		else if(obj->workspace==old)
			do_unmap_winobj(obj);
	}

	set_integer_property(scr->root, wglobal.atom_workspace_num,
						 scr->current_workspace);
}


void do_switch_workspace(int num)
{
	dodo_switch_workspace(num);

	if(wglobal.current_winobj==NULL ||
	   !on_current_workspace(wglobal.current_winobj)){
		if(circulate(1)==NULL){
			do_set_focus((WThing*)SCREEN);
			wglobal.current_winobj=NULL;
		}
	}
}


/* */


void switch_workspace_num(int num)
{
	if(num<0 || num>=SCREEN->n_workspaces)
		return;
	
	do_switch_workspace(num);
}


void switch_workspace_hrot(int dir)
{
	WScreen *scr=SCREEN;
	int num;
	
	num=(scr->current_workspace+dir)%scr->workspaces_horiz;
	if(num<0)
		num+=scr->workspaces_horiz;
	
	num+=(scr->current_workspace/scr->workspaces_horiz)*scr->workspaces_horiz;
	
	do_switch_workspace(num);
}


void switch_workspace_vrot(int dir)
{
	WScreen *scr=SCREEN;
	int num;
	
	num=scr->current_workspace+dir*scr->workspaces_horiz;
	num%=scr->n_workspaces;
	if(num<0)
		num+=scr->n_workspaces;
	
	do_switch_workspace(num);
}


/* */


static void set_client_workspaces(WWinObj *obj)
{
	WClientWin *cwin;
	
	cwin=first_clientwin((WThing*)obj);
	
	while(cwin!=NULL){
		set_integer_property(cwin->client_win, wglobal.atom_workspace_num,
							 obj->workspace);
		cwin=next_clientwin(cwin);
	}
}
		
		
void move_to_workspace(WWinObj *obj, int ws)
{
	WWinObj *tmp;
	int ows;
	
	if(ws>=SCREEN->n_workspaces)
		return;
	
	if(ws<0 && ws!=WORKSPACE_STICKY)
		ws=SCREEN->current_workspace;
	
	tmp=obj;
	
	while(obj!=NULL){
		ows=obj->workspace;
		obj->workspace=ws;
		
		if(visible_workspace(ws)){
			do_map_winobj(obj);
			if(WTHING_IS(obj, WTHING_FRAME) &&
			   (ws==WORKSPACE_STICKY || ows==WORKSPACE_STICKY))
				draw_frame_bar((WFrame*)obj, TRUE);
		}else{
			unmap_winobj(obj);
		}

		if(WTHING_IS(obj, WTHING_FRAME))
			set_client_workspaces(obj);

		obj=traverse_winobjs(obj, tmp);
	}
	
	update_winlist();
}


/* */


void init_workspaces()
{
	int n;
	WScreen *scr=SCREEN;
	CARD32 data[2];
	
	if(scr->n_workspaces<=0){
		scr->n_workspaces=CF_DEFAULT_N_WORKSPACES;
		scr->workspaces_horiz=CF_DEFAULT_N_WORKSPACES;
		scr->workspaces_vert=1;
	}
	
	n=0;
	get_integer_property(scr->root, wglobal.atom_workspace_num, &n);

	if(n>scr->n_workspaces){
		warn("_PWM_WORKSPACE_NUM property out of range. Defaulting to 0.");
		n=0;
		set_integer_property(scr->root, wglobal.atom_workspace_num, 0);
	}
	
	scr->current_workspace=n;

	/* Store virtual desktop dimensions */
	data[0]=SCREEN->workspaces_horiz;
	data[1]=SCREEN->workspaces_vert;
	XChangeProperty(wglobal.dpy, SCREEN->root, wglobal.atom_workspace_info,
					XA_INTEGER, 32, PropModeReplace, (uchar*)data, 2);
}
