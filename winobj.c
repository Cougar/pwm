/*
 * pwm/winobj.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "thing.h"
#include "frame.h"
#include "menu.h"
#include "focus.h"


static Window lowest_win(WWinObj *obj)
{
	if(WTHING_IS(obj, WTHING_FRAME))
		return ((WFrame*)obj)->bar_win;

	if(WTHING_IS(obj, WTHING_DOCK))
		return ((WDock*)obj)->win;
	
	if(WTHING_IS(obj, WTHING_MENU))
		return ((WMenu*)obj)->menu_win;
	
	return None;
}


static Window highest_win(WWinObj *obj)
{
	if(WTHING_IS(obj, WTHING_FRAME))
		return ((WFrame*)obj)->frame_win;

	if(WTHING_IS(obj, WTHING_DOCK))
		return ((WDock*)obj)->win;
	
	if(WTHING_IS(obj, WTHING_MENU))
		return ((WMenu*)obj)->menu_win;
	
	return None;
}


static void do_restack_window(Window win, Window other, int stack_mode)
{
	XWindowChanges wc;
	int wcmask;
	
	wcmask=CWStackMode;
	wc.stack_mode=stack_mode;
	if((wc.sibling=other)!=None)
		wcmask|=CWSibling;

	XConfigureWindow(wglobal.dpy, win, wcmask, &wc);
}


static void restack_windows(WWinObj *obj, Window other, int mode)
{
	WFrame *frame;
	Window win;
	
	win=lowest_win(obj);
	
	do_restack_window(win, other, mode);
	
	if(!WTHING_IS(obj, WTHING_FRAME))
		return;
	
	frame=(WFrame*)obj;
	
	do_restack_window(frame->frame_win, frame->bar_win, Above);
}


void set_winobj_pos(WWinObj *obj, int x, int y)
{
	if(WTHING_IS(obj, WTHING_FRAME))
		set_frame_pos((WFrame*)obj, x, y);
	else if(WTHING_IS(obj, WTHING_DOCK))
		set_dock_pos((WDock*)obj, x, y);
	else if(WTHING_IS(obj, WTHING_MENU))
		set_menu_pos((WMenu*)obj, x, y);
}


bool winobj_is_visible(WWinObj *obj)
{
	if(obj->x>=SCREEN->width)
		return FALSE;

	if(obj->y>=SCREEN->height)
		return FALSE;
	
	if(obj->x+obj->w<=0)
		return FALSE;

	if(obj->y+obj->h<=0)
		return FALSE;
	
	return TRUE;
}


static void focus_stack_above(WWinObj *obj)
{
	wglobal.current_winobj=NULL;
	
	while(1){
		obj=obj->stack_above;
		
		if(obj==NULL)
			break;
		
		if(!WWINOBJ_IS_MAPPED(obj))
			continue;
		
		set_focus_weak((WThing*)obj);
		return;
	}
	
	set_focus_weak((WThing*)SCREEN);
}


/* */


/* Get a pointer to the stacking list that should contain the given
 * winobj and possibly unlink the winobj. The winobj *must* be stacked
 * or else this will fail.
 */
static WWinObj **get_listptr(WWinObj *obj, bool unlink)
{
	WWinObj **listptr=NULL;

	if(obj->stack_lvl!=LVL_OTHER){
		listptr=&(SCREEN->winobj_stack_lists[obj->stack_lvl]);
	}else{
		assert(obj->stack_above!=NULL);
		listptr=&(obj->stack_above->stack_above_list);
	}
	
	if(unlink){
		UNLINK_ITEM(*listptr, obj, stack_next, stack_prev);
	}
	
	return listptr;
}


/* Raise or lower a winobj. 
 */
static void do_raiselower_winobj(WWinObj *obj, bool unlink, int raise)
{
	WScreen *screen=SCREEN;
	Window win, other=None;
	WWinObj **listptr, *tmpobj, *nextobj=NULL;
	int i, mode=Above;
	
	if(!raise && unlink){
		/* If lowering lowest transient, lower parent. */
		while(obj->stack_lvl==LVL_OTHER && obj->stack_prev!=NULL &&
			  obj->stack_prev->stack_next!=obj){
			obj=obj->stack_above;
			assert(obj!=NULL);
		}
	}
	
	tmpobj=obj;
	
	win=lowest_win(obj);
	   
	listptr=get_listptr(obj, unlink);
	assert(listptr!=NULL);
	
	/* Find the "normally" stacked parent of a transient. */
	
	while(tmpobj->stack_above!=NULL)
		tmpobj=tmpobj->stack_above;

	if(!raise && obj->stack_above!=NULL){
		tmpobj=obj->stack_above;
		mode=Above;
		other=highest_win(tmpobj);
	}else{
		/* Find the lowest winobj on this (lower only) or higher stacking
		 * levels. 
		 */
		for(i=tmpobj->stack_lvl+raise; i<N_STACK_LVLS; ++i){
			if(screen->winobj_stack_lists[i]!=NULL)
				break;
		}
	
		/* Found a window to place this one under */
		if(i<N_STACK_LVLS){
			other=lowest_win(screen->winobj_stack_lists[i]);
			mode=Below;
		}
	}
	
	/* Else none were found - this winobj should be raised highest on
	 * the stack (other and mode are already initialized to None and Above).
	 */

	if(raise){
		LINK_ITEM(*listptr, obj, stack_next, stack_prev);
	}else{
		LINK_ITEM_FIRST(*listptr, obj, stack_next, stack_prev);
	}

	tmpobj=init_traverse_winobjs_b(obj, &nextobj);
	
	while(tmpobj!=NULL){
		restack_windows(tmpobj, other, mode);
		other=lowest_win(tmpobj);
		mode=Below;
		tmpobj=traverse_winobjs_b(tmpobj, obj, &nextobj);
	}
}


void raise_winobj(WWinObj *obj)
{
	do_raiselower_winobj(obj, TRUE, 1);
}


void lower_winobj(WWinObj *obj)
{
	do_raiselower_winobj(obj, TRUE, 0);
}


void raiselower_winobj(WWinObj *obj)
{
	do_raiselower_winobj(obj, TRUE, obj->stack_next!=NULL);
}


/* */


static bool check_loop(WWinObj *obj, WWinObj *above)
{
	WWinObj *tmp=obj;
	
	if(obj==above)
		return TRUE;
	
	obj=obj->stack_above_list;
	
	while(obj!=NULL){
		if(obj==above)
			return TRUE;
		obj=traverse_winobjs(obj, tmp);
	}
	return FALSE;
}


static void do_restack_winobj(WWinObj *obj, int stack_lvl,
							  WWinObj *stack_above, bool raiselower)
{
	WWinObj **listptr;

	/* Unlink the object from stacking list unless it isn't
	 * yet stacked (restack_winobj called from add_winobj)
	 */
	if(obj->stack_prev!=NULL)
		get_listptr(obj, TRUE);
	
	obj->stack_lvl=stack_lvl;
	obj->stack_above=stack_above;
	obj->stack_next=NULL;
	obj->stack_prev=NULL;
	
	if(raiselower){
		do_raiselower_winobj(obj, FALSE, 1);
	}else{
		listptr=get_listptr(obj, FALSE);
		LINK_ITEM(*listptr, obj, stack_next, stack_prev);
	}
}


void restack_winobj(WWinObj *obj, int stack_lvl, bool raiselower)
{
	if(stack_lvl<0 || stack_lvl>=N_STACK_LVLS)
		stack_lvl=LVL_NORMAL;

	do_restack_winobj(obj, stack_lvl, NULL, raiselower);
}


void restack_winobj_above(WWinObj *obj, WWinObj *stack_above, bool raiselower)
{
	if(check_loop(obj, stack_above)){
		warn("Attempt to create loop in winobj stack. "
			 "Broken WM_TRANSIENT_FOR hints?");
		do_restack_winobj(obj, LVL_NORMAL, NULL, raiselower);
	}else{
		do_restack_winobj(obj, LVL_OTHER, stack_above, raiselower);
	}
}


void add_winobj(WWinObj *obj, int ws, int stack_lvl)
{
	link_thing((WThing*)SCREEN, (WThing*)obj);
	
	if(ws<0 && ws!=WORKSPACE_STICKY)
		ws=SCREEN->current_workspace;
	
	obj->workspace=ws;
	
	restack_winobj(obj, stack_lvl, TRUE);
}


void add_winobj_above(WWinObj *obj, WWinObj *stack_above)
{
	link_thing((WThing*)SCREEN, (WThing*)obj);
	
	obj->workspace=stack_above->workspace;
	
	restack_winobj_above(obj, stack_above, TRUE);
}


/* */


static void relink_above(WWinObj *obj, WWinObj **listptr)
{
	WWinObj *p, *next, **listptr2;
	
	for(p=obj->stack_above_list; p!=NULL; p=next){
		next=p->stack_next;
		UNLINK_ITEM(obj->stack_above_list, p, stack_next, stack_prev);
		LINK_ITEM(*listptr, p, stack_next, stack_prev);
	}
}


void unlink_winobj_d(WWinObj *obj)
{
	WWinObj **listptr;

	if(wglobal.current_winobj==obj)
		focus_stack_above(obj);

	listptr=get_listptr(obj, TRUE);
	
	destroy_subthings((WThing*)obj);
	unlink_thing((WThing*)obj);
	
	if(wglobal.previous_winobj==obj)
		wglobal.previous_winobj=NULL;
	
	relink_above(obj, listptr);
}


/* */


void do_map_winobj(WWinObj *obj)
{
	WFrame *frame;
	
	if(obj->flags&WWINOBJ_HIDDEN)
		return;

	if(WTHING_IS(obj, WTHING_MENU)){
		XMapWindow(wglobal.dpy, ((WMenu*)obj)->menu_win);
	}else if(WTHING_IS(obj, WTHING_DOCK)){
		XMapWindow(wglobal.dpy, ((WDock*)obj)->win);
	}else if(WTHING_IS(obj, WTHING_FRAME)){
		frame=(WFrame*)obj;
		
		if(!WFRAME_IS_SHADE(frame))
			XMapWindow(wglobal.dpy, frame->frame_win);
		
		if(!WFRAME_IS_NO_BAR(frame))
			XMapWindow(wglobal.dpy, frame->bar_win);
	}
	
	obj->flags|=WWINOBJ_MAPPED;
}


void do_unmap_winobj(WWinObj *obj)
{
	WFrame *frame;

	if(!(obj->flags&WWINOBJ_MAPPED))
		return;

	if(WTHING_IS(obj, WTHING_MENU)){
		XUnmapWindow(wglobal.dpy, ((WMenu*)obj)->menu_win);
	}else if(WTHING_IS(obj, WTHING_DOCK)){
		XUnmapWindow(wglobal.dpy, ((WDock*)obj)->win);
	}else if(WTHING_IS(obj, WTHING_FRAME)){
		frame=(WFrame*)obj;
		XUnmapWindow(wglobal.dpy, frame->frame_win);
		XUnmapWindow(wglobal.dpy, frame->bar_win);
	}

	obj->flags&=~WWINOBJ_MAPPED;
}


void map_winobj(WWinObj *obj)
{
	if(!on_current_workspace(obj))
		return;
	
	do_map_winobj(obj);
}


void unmap_winobj(WWinObj *obj)
{
	do_unmap_winobj(obj);

	if(wglobal.current_winobj==obj){
		wglobal.current_winobj=NULL;
		focus_stack_above(obj);
	}
}


/* */


/* Will traverse through a tree of winobjs each branch at a time.
 * Usage:
 * 
 * 	WWinObj *root;
 * 
 *  root=obj;
 *  # obj=root->stack_above_list; if root is not to be included 
 *
 * 	while(obj!=NULL){
 * 		do the stuff on obj
 * 		obj=traverse_winobjs(obj, root);
 *  }
 */

WWinObj* traverse_winobjs(WWinObj *current, WWinObj *root)
{
	if(current==NULL)
		return NULL;

	if(current->stack_above_list!=NULL)
		return current->stack_above_list;
	
	if(current==root)
		return NULL;
	
	while(current->stack_next==NULL){
		current=current->stack_above;
		
		if(current==root)
			return NULL;

		assert(current!=NULL);
	}

	return current->stack_next;
}


static WWinObj *find_top_node(WWinObj *p)
{	
	while(p->stack_above_list!=NULL){
		p=p->stack_above_list;
		p=p->stack_prev;
	}
	
	return p;
}


/* Will traverse through a tree backwards (starting from the topmost
 * item in the last branch). The "root" winobj is included; last of course.
 * Usage:
 * 
 * 	WWinObj *helper, *obj;
 * 
 *  obj=init_traverse_winobjs_b(foo, &helper);
 * 
 * 	while(obj!=NULL){
 * 		do the stuff on obj
 * 		obj=traverse_winobjs_b(obj, foo, &helper);
 *  }
 */

WWinObj* init_traverse_winobjs_b(WWinObj *root, WWinObj **next)
{
	WWinObj *p;
	
	p=root;
	
	if(p->stack_above_list==NULL){
		*next=NULL;
		return p;
	}
	
	p=find_top_node(p);

	if(p->stack_prev!=p)
		*next=p->stack_prev;
	else
		*next=p->stack_above;

	return p;
}


WWinObj* traverse_winobjs_b(WWinObj *p, WWinObj *root, WWinObj **next)
{
	WWinObj *tmp=NULL;
	
	if(p==NULL || *next==NULL || p==root)
		return NULL;

	if(*next==p->stack_above){
		tmp=(*next)->stack_next;
		if(tmp!=NULL)
			tmp=find_top_node(tmp);
		else
			tmp=(*next)->stack_above;
	}else if((*next)->stack_prev->stack_next==NULL){
			tmp=(*next)->stack_above;
	}else{
			tmp=(*next)->stack_prev;
	}

	p=*next;
	*next=tmp;

	return p;
}
