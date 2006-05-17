/*
 * pwm/thing.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include "common.h"
#include "thing.h"
#include "frame.h"
#include "clientwin.h"


void destroy_thing(WThing *t)
{
	if(WTHING_IS(t, WTHING_FRAME))
		destroy_frame((WFrame*)t);
	else if(WTHING_IS(t, WTHING_DOCK))
		destroy_dock((WDock*)t);
	else if(WTHING_IS(t, WTHING_MENU))
		destroy_menu_tree((WMenu*)t);
	else if(WTHING_IS(t, WTHING_CLIENTWIN))
		unmanage_clientwin((WClientWin*)t);
}


void free_thing(WThing *t)
{
	if(wglobal.focus_next==t)
		wglobal.focus_next=NULL/*(WThing*)SCREEN*/;
	if(wglobal.grab_holder==t)
		wglobal.grab_holder=NULL;
	free(t);
}


void destroy_subthings(WThing *parent)
{
	WThing *t, *prev=NULL;

	/* assert(!(parent->flags&WTHING_SUBDEST)); */
    if (parent->flags & WTHING_SUBDEST) {
        fprintf(stderr, __FILE__ ": %d: destroy_subthings: "
            "parent->flags & WTHING_SUBDEST\n", __LINE__);
        /* Do nothing - we're in some weird trouble here. */
    } else {
        parent->flags |= WTHING_SUBDEST;
	
	/* destroy children */
        while ((t = parent->t_children) != NULL) {
		assert(t!=prev);
		prev=t;
		destroy_thing(t);
	}
	
	parent->flags&=~WTHING_SUBDEST;
    }
}


/* */


void link_thing(WThing *parent, WThing *thing)
{
	LINK_ITEM(parent->t_children, thing, t_next, t_prev);
	thing->t_parent=parent;
}


void link_thing_before(WThing *before, WThing *thing)
{
	WThing *parent=before->t_parent;
	LINK_ITEM_BEFORE(parent->t_children, before, thing, t_next, t_prev);
	thing->t_parent=parent;
}


void unlink_thing(WThing *thing)
{
	WThing *parent=thing->t_parent;

	if(parent==NULL)
		return;
	
	UNLINK_ITEM(parent->t_children, thing, t_next, t_prev);
}


/* */


static WThing *get_next_thing(WThing *first, int filt)
{
	while(first!=NULL){
		if(filt==0 || WTHING_IS(first, filt))
			break;
		first=first->t_next;
	};
	
	return first;
}


WThing *next_thing(WThing *first, int filt)
{
	if(first==NULL)
		return NULL;
	
	return get_next_thing(first->t_next, filt);
}

static WThing *get_prev_thing(WThing *first, int filt)
{
	while(first!=NULL){
		if(filt==0 || WTHING_IS(first, filt))
			break;
		first=first->t_prev;
	};

	return first;
}

WThing *prev_thing(WThing *first, int filt)
{
	if(first==NULL)
		return NULL;

	return get_prev_thing(first->t_prev, filt);
}
	
WThing *subthing(WThing *parent, int filt)
{
	if(parent==NULL)
		return NULL;
	
	return get_next_thing(parent->t_children, filt);
}


WClientWin *next_clientwin(WClientWin *cwin)
{
	return (WClientWin*)next_thing((WThing*)cwin, WTHING_CLIENTWIN);
}


WClientWin *prev_clientwin(WClientWin *cwin)
{
	return (WClientWin*)prev_thing((WThing*)cwin, WTHING_CLIENTWIN);
}

WClientWin *first_clientwin(WThing *thing)
{
	return (WClientWin*)subthing(thing, WTHING_CLIENTWIN);
}


WThing *nth_thing(WThing *first, int num)
{
	if(first==NULL)
		return NULL;
	
	while(num!=0){
		if(num<0){
			num++;
			first=first->t_prev;
		}else if(num>0){
			num--;
			if(first->t_next!=NULL)
				first=first->t_next;
			else
				first=first->t_parent->t_children;
		}
		if(num==0)
			break;
	}

	return first;
}

	
WThing *nth_subthing(WThing *parent, int num)
{
	if(parent==NULL)
		return NULL;
	
	return nth_thing(parent->t_children, num);
}
	

/* */


WThing *find_thing(Window win)
{
	WThing *thing;
	
	if(XFindContext(wglobal.dpy, win, wglobal.win_context,
					(XPointer*)&thing)!=0)
		return NULL;
	
	return thing;
}


WThing *find_thing_t(Window win, int type)
{
	WThing *thing=find_thing(win);
	
	if(thing==NULL || !WTHING_IS(thing, type))
		return NULL;
	
	return thing;
}


WClientWin *find_clientwin(Window win)
{
	return (WClientWin*)find_thing_t(win, WTHING_CLIENTWIN);
}


/* */


WFrame *find_frame_of(Window win)
{
	WWinObj *winobj=find_winobj_of(win);
	
	if(winobj==NULL || !WTHING_IS(winobj, WTHING_FRAME))
		return NULL;
	
	return (WFrame*)winobj;
}


WWinObj *find_winobj_of(Window win)
{
	WThing *thing=find_thing(win);
	
	if(thing==NULL)
		return NULL;
	
	if(WTHING_IS(thing, WTHING_FRAME))
		return (WWinObj*)thing;

	if(WTHING_IS(thing, WTHING_CLIENTWIN) && thing->t_parent!=NULL &&
	   WTHING_IS(thing->t_parent, WTHING_WINOBJ))
		return (WWinObj*)thing->t_parent;
	
	return NULL;
}


/* */


WWinObj *winobj_of(WThing *thing)
{
	if(WTHING_IS(thing, WTHING_WINOBJ))
		return (WWinObj*)thing;
	
	if(WTHING_IS(thing, WTHING_CLIENTWIN))
		return (WWinObj*)CWIN_FRAME((WClientWin*)thing);
	
	return NULL;
}
