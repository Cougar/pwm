/*
 * pwm/frame.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "frame.h"
#include "frameid.h"
#include "clientwin.h"
#include "screen.h"
#include "draw.h"
#include "property.h"
#include "winobj.h"
#include "focus.h"
#include "event.h"
#include "binding.h"
#include "moveres.h"
#include "placement.h"
#include "winlist.h"


#define IS_CURRENT_FRAME(FRAME) ((WWinObj*)(FRAME)==wglobal.current_winobj)


static bool do_frame_detach_clientwin(WFrame *frame, WClientWin *cwin,
									  WFrame *transient_dest);
static void check_minmax_size(WFrame *frame, WClientWin *cwin);
static void fit_clientwin(WFrame *frame, WClientWin *cwin);
static void move_transients_to(WFrame *dest, WFrame *src,
							   Window transient_for);
static void hideshow_transients(WFrame *dest,
								Window show_for, Window hide_for);


/* */


void clientwin_to_frame_size(int iw, int ih, int flags, int *fw, int *fh)
{
	if(!(flags&WFRAME_NO_BORDER)){
		iw+=2*CF_BORDER_WIDTH;
		ih+=2*CF_BORDER_WIDTH;
	}
	
	if(!(flags&WFRAME_NO_BAR))
		ih+=GRDATA->bar_height;
	
	*fw=iw;
	*fh=ih;
}


/* 
 * Frame creation
 */


WFrame* create_frame(int x, int y, int iw, int ih, int id, int flags)
{
	WFrame *frame;
	int w, h;
	Window frame_win, bar_win;
	
	frame=ALLOC(WFrame);
	
	if(frame==NULL){
		warn_err();
		return NULL;
	}
	
	WTHING_INIT(frame, WTHING_FRAME);
	
	flags&=WFRAME_NO_BAR|WFRAME_NO_BORDER;
	flags|=WFRAME_HIDDEN|WFRAME_SHADE;
	
	frame->flags=flags;
	frame->frame_id=(id==0 ? new_frame_id() : use_frame_id(id));
	frame->frame_iw=iw;
	frame->frame_ih=ih;
	frame->frame_ix=frame->frame_iy=(flags&WFRAME_NO_BORDER ? 0 :
									 CF_BORDER_WIDTH);
	frame->min_w=CF_WIN_MIN_WIDTH;
	frame->min_h=CF_WIN_MIN_HEIGHT;
	frame->max_w=-1;
	frame->max_h=-1;
	frame->cwin_count=0;
	
	clientwin_to_frame_size(iw, ih, flags, &w, &h);
	
	frame->x=x;
	frame->y=y;
	frame->w=w;
	frame->h=h;
	frame->bar_w=frame->tab_w=frame->w;
	frame->bar_h=(flags&WFRAME_NO_BAR ? 0 : GRDATA->bar_height);
	
#ifdef CF_WANT_TRANSPARENT_TERMS
	frame_win=XCreateWindow(wglobal.dpy, SCREEN->root,
							FRAME_X(frame), FRAME_Y(frame),
							FRAME_W(frame), FRAME_H(frame),
							0, CopyFromParent, InputOutput, CopyFromParent,
							0, NULL);
#else
	frame_win=create_simple_window(FRAME_X(frame), FRAME_Y(frame),
								   FRAME_W(frame), FRAME_H(frame),
								   GRDATA->base_colors.pixels[WCG_PIX_BG]);
#endif
	bar_win=create_simple_window(BAR_X(frame), BAR_Y(frame),
								 BAR_W(frame), GRDATA->bar_height, 
								 GRDATA->tab_colors.pixels[WCG_PIX_BG]);
	
	frame->frame_win=frame_win;
	frame->bar_win=bar_win;

	XSelectInput(wglobal.dpy, frame_win, FRAME_MASK);
	XSaveContext(wglobal.dpy, frame_win, wglobal.win_context, (XPointer)frame);

	XSelectInput(wglobal.dpy, bar_win, BAR_MASK);
	XSaveContext(wglobal.dpy, bar_win, wglobal.win_context, (XPointer)frame);
	
	grab_bindings(frame_win, ACTX_WINDOW|ACTX_GLOBAL);
	
	return frame;
}


WFrame *create_add_frame_simple(int root_x, int root_y, int iiw, int iih)
{
	WFrame *frame;

	frame=create_frame(root_x, root_y, iiw, iih, 0, 0);
	
	if(frame!=NULL)
		add_winobj((WWinObj*)frame, WORKSPACE_CURRENT, LVL_NORMAL);
	
	return frame;
}


/* 
 * Frame destroy stuff
 */


void destroy_frame(WFrame *frame)
{
	unlink_winobj_d((WWinObj*)frame);

	XDeleteContext(wglobal.dpy, frame->frame_win, wglobal.win_context);
	XDeleteContext(wglobal.dpy, frame->bar_win, wglobal.win_context);
	
	XDestroyWindow(wglobal.dpy, frame->frame_win);
	XDestroyWindow(wglobal.dpy, frame->bar_win);
	
	free_frame_id(frame->frame_id);
	
	free_thing((WThing*)frame);
}


/* 
 * Clientwin switching stuff
 */


static void do_frame_switch_clientwin(WFrame *frame, WClientWin *cwin,
									  bool simple)
{
	if(!WFRAME_IS_NORMAL(frame)){
		frame->current_cwin=cwin;
		if(!simple && WFRAME_IS_SHADE(frame))
			draw_frame_bar(frame, TRUE);
		
		update_winlist();
		return;
	}

#if 0
	fit_clientwin(frame, cwin);
#endif	
	show_clientwin(cwin);
	
	if(!simple)
		hide_clientwin(frame->current_cwin);
	
	hideshow_transients(frame, cwin->client_win,
						simple ? None : frame->current_cwin->client_win);

	frame->current_cwin=cwin;

	if(IS_CURRENT_FRAME(frame))
		set_focus_weak((WThing*)cwin);

	if(!simple)
		draw_frame_bar(frame, TRUE);
	
	update_winlist();
}


void frame_switch_clientwin(WFrame *frame, WClientWin *cwin)
{
	if(cwin!=frame->current_cwin)
		do_frame_switch_clientwin(frame, cwin, FALSE);
}


void frame_switch_nth(WFrame *frame, int num)
{
	WThing *thing;
	
	thing=nth_subthing((WThing*)frame, num);
	
	if(thing!=NULL)
		frame_switch_clientwin(frame, (WClientWin*)thing);
}


void frame_switch_rot(WFrame *frame, int rotcnt)
{
	WThing *thing;
	
	if(frame->current_cwin==NULL)
		thing=nth_subthing((WThing*)frame, rotcnt);
	else
		thing=nth_thing((WThing*)(frame->current_cwin), rotcnt);
	
	if(thing!=NULL)
		frame_switch_clientwin(frame, (WClientWin*)thing);
}


/* 
 * Frame state
 */


void set_frame_state(WFrame *frame, int stateflags)
{
	int changes;
	Window show_for=None, hide_for=None;
	/*
	if(WFRAME_IS_NO_BAR(frame))
		stateflags&=~WFRAME_HIDDEN;

	if(WFRAME_IS_NO_BORDER(frame))
		stateflags&=~WFRAME_SHADE;
	*/
	changes=(stateflags^frame->flags)&(WFRAME_HIDDEN|WFRAME_SHADE);
	
	if(changes==0)
		return;
	
	if(WFRAME_IS_NORMAL(frame)){
		hide_clientwin(frame->current_cwin);
		hide_for=frame->current_cwin->client_win;
	}

	frame->flags^=changes; 

	if(stateflags&WFRAME_HIDDEN){
		/* hide it */
		if(changes&WFRAME_HIDDEN)
			unmap_winobj((WWinObj*)frame);
	}else if(stateflags&WFRAME_SHADE){
		/* shade it */
		if(WWINOBJ_IS_MAPPED(frame))
			XUnmapWindow(wglobal.dpy, frame->frame_win);
		else
			map_winobj((WWinObj*)frame);

		if(IS_CURRENT_FRAME(frame))
			set_focus_weak((WThing*)frame);
	}else{
		/* show it */
		if(WWINOBJ_IS_MAPPED(frame))
			XMapWindow(wglobal.dpy, frame->frame_win);
		else
			map_winobj((WWinObj*)frame);
		
		show_clientwin(frame->current_cwin);
		show_for=frame->current_cwin->client_win;
		
		if(IS_CURRENT_FRAME(frame))
			set_focus_weak((WThing*)frame);
	}
	
	hideshow_transients(frame, show_for, hide_for);
}


/*
 * Focus
 */


void activate_frame(WFrame *frame)
{
	draw_frame_bar(frame, TRUE);
	draw_frame_frame(frame, TRUE);
}


void deactivate_frame(WFrame *frame)
{
	draw_frame_bar(frame, TRUE);
	draw_frame_frame(frame, TRUE);
}


/* */


static void restack_transient(WFrame *frame, Window transient_for)
{
	WClientWin *cwin=find_clientwin(transient_for);
	
	if(cwin==NULL || !CWIN_HAS_FRAME(cwin))
		return;
					   
	restack_winobj_above((WWinObj*)frame, (WWinObj*)CWIN_FRAME(cwin),
						 FALSE);
}


static void frame_reconf_clientwin(WFrame* frame, WClientWin *cwin)
{
	int x, y;
	
	x=FRAME_X(frame)+frame->frame_ix;
	y=FRAME_Y(frame)+frame->frame_iy;
#ifndef CF_CWIN_TOPLEFT
	x+=(frame->frame_iw-cwin->client_w)/2;
	y+=(frame->frame_ih-cwin->client_h)/2;
#endif
	clientwin_reconf_at(cwin, x, y);
}


/*
 * Attach new window to a frame.
 */

static void do_frame_attach_clientwin(WFrame *frame, WClientWin *cwin)
{
	int tw=frame->frame_iw, th=frame->frame_ih;

	link_thing((WThing*)frame, (WThing*)cwin);
	frame->cwin_count++;

	set_integer_property(cwin->client_win, wglobal.atom_frame_id,
						 frame->frame_id);
	set_integer_property(cwin->client_win, wglobal.atom_workspace_num,
						 frame->workspace);
	
	check_minmax_size(frame, cwin);
	if(frame->frame_iw!=cwin->client_w || frame->frame_ih!=cwin->client_h)
		frame_clientwin_resize(frame, cwin, frame->frame_iw, frame->frame_ih,
							   FALSE);
	else
		frame_recalc_bar(frame);
	
	if(cwin->state!=IconicState && WFRAME_IS_SHADE(frame)){
		frame_switch_clientwin(frame, cwin);
		set_frame_state(frame, frame->flags&~WFRAME_SHADE);
	}else if(frame->cwin_count==1){
		if(cwin->state==IconicState)
			set_frame_state(frame, frame->flags|WFRAME_SHADE);
		frame_switch_clientwin(frame, cwin);
	}

	if(!WFRAME_IS_NORMAL(frame) || cwin!=frame->current_cwin)
		hide_clientwin(cwin);
}


bool frame_attach_clientwin(WFrame *frame, WClientWin *cwin)
{
	WFrame *oframe=NULL;
	WWinObj *tmp;
	
	if(CWIN_HAS_FRAME(cwin)){
		if(CWIN_FRAME(cwin)==frame)
			return TRUE;

		oframe=CWIN_FRAME(cwin);
	}

	if(frame->cwin_count==1 && frame->stack_above!=NULL){
		/* Adding new window to a transient-stacked frame -> 
		 * stack normally.
		 */
		tmp=(WWinObj*)frame;
		do{
			tmp=tmp->stack_above;
		}while(tmp->stack_above!=NULL);
		
		restack_winobj((WWinObj*)frame, tmp->stack_lvl, FALSE);
	}

#ifndef CF_NO_WILD_WINDOWS
	if(frame->cwin_count==0 && CWIN_IS_WILD(cwin))
		frame_set_decor(frame, WFRAME_NO_DECOR);
	else
		frame_set_decor(frame, 0);
#endif

	/* Reparent and attach the window */
	XSelectInput(wglobal.dpy, cwin->client_win,
				 cwin->event_mask&~StructureNotifyMask);
	XReparentWindow(wglobal.dpy, cwin->client_win, frame->frame_win,
					frame->frame_ix, frame->frame_iy);
	XSelectInput(wglobal.dpy, cwin->client_win, cwin->event_mask);

	/* Tk apps need this or else their menus will be initially fsck'd up */
	frame_reconf_clientwin(frame, cwin);
	
	if(oframe!=NULL){
		if(!do_frame_detach_clientwin(oframe, cwin, frame))
			oframe=NULL;
	}

	do_frame_attach_clientwin(frame, cwin);

	if(frame->cwin_count==1 && cwin->transient_for!=None){
		/* Adding a new transient window to an empty frame? */
		restack_transient(frame, cwin->transient_for);
	}
	
	if(oframe!=NULL && oframe->cwin_count==1){
		cwin=first_clientwin((WThing*)oframe);
		assert(cwin!=NULL);
		
		if(cwin->transient_for!=None)
			restack_transient(oframe, cwin->transient_for);
		
#ifndef CF_NO_WILD_WINDOWS		
		if(CWIN_IS_WILD(cwin))
			frame_set_decor(oframe, WFRAME_NO_DECOR);
#endif		
	}

	update_winlist();
	
	return TRUE;
}


/*
 * Detach a window from a frame
 */
		
static bool do_frame_detach_clientwin(WFrame *frame, WClientWin *cwin,
									  WFrame *transient_dest)
{
	WClientWin *next;
	int nw, nh;
	
#if 0
	next=prev_clientwin(cwin);
	if(next==NULL)
		next=next_clientwin(cwin);
#else
	next=next_clientwin(cwin);
	if(next==NULL)
		next=prev_clientwin(cwin);
#endif	
	move_transients_to(transient_dest, frame, cwin->client_win);

	unlink_thing((WThing*)cwin);
	frame->cwin_count--;
	
	if(frame->cwin_count==0){
		if(!(frame->flags&WTHING_SUBDEST))
			destroy_frame(frame);
		return FALSE;
	}

	assert(next!=NULL && next!=cwin);

	nw=frame->frame_iw;
	nh=frame->frame_ih;

	frame_recalc_minmax(frame);

	if(!FRAME_MAXW_UNSET(frame) && frame->frame_iw>frame->max_w)
		nw=frame->max_w;
	if(!FRAME_MAXH_UNSET(frame) && frame->frame_ih>frame->max_h)
		nh=frame->max_h;

	if(cwin==frame->current_cwin)
		do_frame_switch_clientwin(frame, next, TRUE);
	
	if(nw!=frame->frame_iw || nh!=frame->frame_ih)
		set_frame_size(frame, nw, nh);
	else
		frame_recalc_bar(frame);
	
	return TRUE;
}


void frame_detach_clientwin(WFrame *frame, WClientWin *cwin, int x, int y)
{
	XSelectInput(wglobal.dpy, cwin->client_win,
				 cwin->event_mask&~StructureNotifyMask);
	XReparentWindow(wglobal.dpy, cwin->client_win, SCREEN->root, x, y);
	XSelectInput(wglobal.dpy, cwin->client_win, cwin->event_mask);

	if(do_frame_detach_clientwin(frame, cwin, NULL) && frame->cwin_count==1){
		cwin=first_clientwin((WThing*)frame);
		assert(cwin!=NULL);
		
		if(cwin->transient_for!=None)
			restack_transient(frame, cwin->transient_for);
		
#ifndef CF_NO_WILD_WINDOWS		
		if(CWIN_IS_WILD(cwin))
			frame_set_decor(frame, WFRAME_NO_DECOR);
#endif		
	}
	
	update_winlist();
}


/* 
 * Move and resize
 */


static void do_set_frame_size(WFrame *frame, int iw, int ih)
{
	frame->frame_iw=iw;
	frame->frame_ih=ih;
	
	clientwin_to_frame_size(iw, ih, frame->flags, &(frame->w), &(frame->h));
	
	XResizeWindow(wglobal.dpy, frame->frame_win,
				  FRAME_W(frame), FRAME_H(frame));

	frame_recalc_bar(frame);
}


void set_frame_size(WFrame *frame, int iw, int ih)
{
	WClientWin *cwin;
	
	do_set_frame_size(frame, iw, ih);
	
	frame->flags&=~WFRAME_MAX_BOTH;
	
	cwin=first_clientwin((WThing*)frame);
	
	while(cwin!=NULL){
		fit_clientwin(frame, cwin);
		cwin=next_clientwin(cwin);
	}
}


void set_frame_pos(WFrame *frame, int x, int y)
{
	WClientWin *cwin;
	
	frame->x=x;
	frame->y=y;
	XMoveWindow(wglobal.dpy, frame->bar_win, BAR_X(frame), BAR_Y(frame));
	XMoveWindow(wglobal.dpy, frame->frame_win, FRAME_X(frame), FRAME_Y(frame));

	cwin=first_clientwin((WThing*)frame);
	while(cwin!=NULL){
		frame_reconf_clientwin(frame, cwin);
		cwin=next_clientwin(cwin);
	}
}


static void check_minmax_size(WFrame *frame, WClientWin *cwin)
{
	if(cwin->size_hints.flags&PMinSize){
		if(cwin->size_hints.min_width>frame->min_w)
			frame->min_w=cwin->size_hints.min_width;
		if(cwin->size_hints.min_height>frame->min_h)
			frame->min_h=cwin->size_hints.min_height;
	}
	
	if(cwin->size_hints.flags&PMaxSize){
		if(frame->max_w!=0 && cwin->size_hints.max_width>frame->max_w)
			frame->max_w=cwin->size_hints.max_width;
		if(frame->max_h!=0 && cwin->size_hints.max_height>frame->max_h)
			frame->max_h=cwin->size_hints.max_height;
	}else{
		frame->max_w=0;
		frame->max_h=0;
	}
	
	if(!FRAME_MAXW_UNSET(frame) && frame->min_w>frame->max_w)
		frame->max_w=frame->min_w;
	
	if(!FRAME_MAXH_UNSET(frame) && frame->min_h>frame->max_h)
		frame->max_h=frame->min_h;
}


void frame_recalc_minmax(WFrame *frame)
{	
	WClientWin *cwin=first_clientwin((WThing*)frame);
	
	frame->min_w=CF_WIN_MIN_WIDTH;
	frame->min_h=CF_WIN_MIN_HEIGHT;
	frame->max_w=-1;
	frame->max_h=-1;
		
	while(cwin!=NULL){
		check_minmax_size(frame, cwin);
		cwin=next_clientwin(cwin);
	}
}


static void do_resize_clientwin(WFrame *frame, WClientWin *cwin, int w, int h)
{
#ifndef CF_CWIN_TOPLEFT
	int x, y;
#endif

	if(w!=cwin->client_w || h!=cwin->client_h){
		cwin->client_w=w;
		cwin->client_h=h;
		XResizeWindow(wglobal.dpy, cwin->client_win, w, h);
	}
	
#ifndef CF_CWIN_TOPLEFT
	x=frame->frame_ix+(frame->frame_iw-cwin->client_w)/2;
	y=frame->frame_iy+(frame->frame_ih-cwin->client_h)/2;
	XMoveWindow(wglobal.dpy, cwin->client_win, x, y);
	clientwin_reconf_at(cwin, FRAME_X(frame)+x, FRAME_Y(frame)+y);
#endif
}


static void fit_clientwin(WFrame *frame, WClientWin *cwin)
{
	int w=frame->frame_iw;
	int h=frame->frame_ih;

	calc_size(frame, cwin, FALSE, &w, &h);
	do_resize_clientwin(frame, cwin, w, h);
}


/* This is called when a client wants its window resized.
 */
void frame_clientwin_resize(WFrame *frame, WClientWin *cwin, int w, int h,
							bool dmax)
{
	int fw, fh;
	WClientWin *tmp;
	int tmpf=frame->flags;
	
	calc_size(frame, cwin, FALSE, &w, &h);
	
	fw=w;
	fh=h;
	
	if(frame->cwin_count>1){
		/* Don't resize the frame smaller then the biggest window */
		for(tmp=first_clientwin((WThing*)frame);
			tmp!=NULL;
			tmp=next_clientwin(tmp)){
			
			if(tmp==cwin)
				continue;
			if(tmp->client_w>fw)
				fw=tmp->client_w;
			if(tmp->client_h>fh)
				fh=tmp->client_h;
		}
	}
	
	
	do_set_frame_size(frame, fw, fh);
	
	if(!dmax)
		frame->flags=tmpf;
	
	/* Resize all the windows */
	tmp=first_clientwin((WThing*)frame);
	while(tmp!=NULL){
		if(tmp==cwin)
			do_resize_clientwin(frame, tmp, w, h);
		else			
			fit_clientwin(frame, tmp);
		tmp=next_clientwin(tmp);
	}
}


/* Possibly take dock into account? */
#define MAX_WIDTH (SCREEN->width)
#define MAX_HEIGHT (SCREEN->height)


static void maximize_frame(WFrame *frame, int flags)
{
	int ow, oh;
	int ox, oy;
	int w, h;
	int x, y;
	
	ox=frame->x;
	oy=frame->y;
	ow=frame->frame_iw;
	oh=frame->frame_ih;
	
	if(flags&WFRAME_MAX_HORIZ){
		w=MAX_WIDTH-frame->frame_ix*2;
		x=0;

		frame->saved_iw=ow;
		frame->saved_x=ox;
	}else{
		x=ox;
		w=ow;
	}

	if(flags&WFRAME_MAX_VERT){
		h=MAX_HEIGHT-frame->frame_iy*2-frame->bar_h;
		y=0;
		
		frame->saved_ih=oh;
		frame->saved_y=oy;
	}else{
		y=oy;
		h=oh;
	}
	
	flags|=frame->flags;
	
	calc_size(frame, frame->current_cwin, frame->cwin_count!=1, &w, &h);
	
	set_frame_pos(frame, x, y);
	set_frame_size(frame, w, h);
	
	frame->flags=flags;
}


static void unmaximize_frame(WFrame *frame, int flags)
{
	int x, y;
	int w, h;
	
	if(flags&WFRAME_MAX_HORIZ){
		x=frame->saved_x;
		w=frame->saved_iw;
	}else{
		x=frame->x;
		w=frame->frame_iw;
	}

	if(flags&WFRAME_MAX_VERT){
		y=frame->saved_y;
		h=frame->saved_ih;
	}else{
		y=frame->y;
		h=frame->frame_ih;
	}
	
	flags=frame->flags&~flags;

	calc_size(frame, frame->current_cwin, frame->cwin_count!=1, &w, &h);
	
	set_frame_size(frame, w, h);
	set_frame_pos(frame, x, y);
	
	frame->flags=flags;
}


static void frame_do_toggle_maximize(WFrame *frame, int mask)
{
	int f=mask&~frame->flags;

	if(f==0)
		unmaximize_frame(frame, mask&frame->flags);
	else
		maximize_frame(frame, f);
}


void frame_toggle_maximize(WFrame *frame, int mask)
{
	mask&=WFRAME_MAX_BOTH;
	
	if(mask==0)
		mask=WFRAME_MAX_BOTH;
	
	frame_do_toggle_maximize(frame, mask);
}


/*
 * Transient stuff
 */


static void hideshow_transients(WFrame *frame,
								Window show_for, Window hide_for)
{
	WWinObj *obj;
	WClientWin *cwin;

	if(hide_for==show_for)
		return;

	obj=frame->stack_above_list;

	while(obj!=NULL){
		if(obj->type!=WTHING_FRAME)
			goto cont;

		/* There should only be exactly one client window if it is
		 * a transient. */
		cwin=first_clientwin((WThing*)obj);
		
		if(cwin==NULL)
			continue;

		if(cwin->transient_for==hide_for && hide_for!=None)
			set_frame_state((WFrame*)obj, obj->flags|WFRAME_HIDDEN);
		else if(cwin->transient_for==show_for && show_for!=None)
			set_frame_state((WFrame*)obj, obj->flags&~WFRAME_HIDDEN);
	cont:
		obj=traverse_winobjs(obj, (WWinObj*)frame);
	}
}


static void move_transients_to(WFrame *dest, WFrame *src, Window transient_for)
{
	WWinObj *obj, *next;
	WClientWin *cwin=NULL;
	
	for(obj=src->stack_above_list; obj!=NULL; obj=next){
		next=obj->stack_next;
		
		if(obj->type!=WTHING_FRAME)
			continue;
		
		/* A window is being attached to a frame containing its
		 * transient windows - this should be taken care of elsewhere
		 */
		assert((WFrame*)obj!=dest);

		/* There should only be exactly one client window if it is
		 * a transient.
		 */
		cwin=first_clientwin((WThing*)obj);
		if(cwin==NULL)
			continue;
		
		if(cwin->transient_for!=transient_for)
			continue;
		   
		if(dest==NULL){
			restack_winobj(obj, src->stack_lvl, FALSE /*TRUE*/);
			set_frame_state((WFrame*)obj, obj->flags&~WFRAME_HIDDEN);
		}else{
			restack_winobj_above(obj, (WWinObj*)dest, TRUE);
			/* Hide now, no need to run through the list again later */
			if(dest->cwin_count>0)
				set_frame_state((WFrame*)obj, obj->flags|WFRAME_HIDDEN);
		}
		
	}
}


/* 
 * Tagged-clientwin stuff
 */


static void do_attach_tagged(WFrame *frame)
{
	WClientWin *cwin;
	bool newframe=FALSE;
	int x, y;
	
	cwin=SCREEN->clientwin_list;
	
	for(; cwin!=NULL; cwin=cwin->s_cwin_next){
		if(!(cwin->flags&CWIN_TAGGED))
			continue;
		
		cwin->flags&=~CWIN_TAGGED;
		
		if(frame==NULL){
			frame=create_add_frame_simple(0, 0,
										  cwin->client_w, cwin->client_h);
			if(frame==NULL)
				return;
			newframe=TRUE;
		}
		frame_attach_clientwin(frame, cwin);
	}
	
	if(newframe){
		calc_placement(frame->w, frame->h, frame->workspace, &x, &y);
		set_frame_pos(frame, x, y);
		set_frame_state(frame, 0);
	}
}


void frame_attach_tagged(WFrame *frame)
{
	do_attach_tagged(frame);
}


void join_tagged()
{
	do_attach_tagged(NULL);
}


/* 
 * Misc.
 */


void frame_recalc_bar(WFrame *frame)
{
	WClientWin *cwin;
	const char *str;
	int maxw=0;
	int tmaxw=0;
	int w, tmp;
	int n=0;
	
	if(frame->flags&WFRAME_NO_BAR)
		return;
	
	for(cwin=first_clientwin((WThing*)frame);
		cwin!=NULL;
		cwin=next_clientwin(cwin)){
		
		if(cwin->state!=NormalState && cwin->state!=IconicState)
			continue;
		   
		str=clientwin_full_label(cwin);
		
		w=XTextWidth(GRDATA->font, str, strlen(str));
		
		if(w>tmaxw)
			tmaxw=w;
	
		n++;
	}
		
	assert(n!=0);

	maxw=CF_BAR_MAX_WIDTH_Q*frame->w;
	if(maxw<CF_BAR_MIN_WIDTH && frame->w>=CF_BAR_MIN_WIDTH)
		maxw=CF_BAR_MIN_WIDTH;
	
	w=(tmaxw+CF_TAB_MIN_TEXT_X_OFF*2);
	tmp=maxw-w*n;	/* tmp=bar max width - min space for full-label tabs */
	
	if(tmp>0){
		/* No label truncation needed. Good. See how much can be padded. */
		tmp/=n*2;
		
		if(tmp>CF_TAB_MAX_TEXT_X_OFF)
			tmp=CF_TAB_MAX_TEXT_X_OFF;
		
		w=tmaxw+tmp*2;
		
		if(w<CF_TAB_MIN_WIDTH){
			w=CF_TAB_MIN_WIDTH;
			if(w*n>maxw)
				w=maxw/n;
		}
	}else{
		/* Some labels must be truncated. */
		w=maxw/n;
	}
	
	tmaxw=w-2*CF_TAB_MIN_TEXT_X_OFF;

	/* Make the labels */
	for(cwin=first_clientwin((WThing*)frame);
		cwin!=NULL;
		cwin=next_clientwin(cwin)){
		
		if(cwin->state!=NormalState && cwin->state!=IconicState)
			continue;
	   
		clientwin_make_label(cwin, tmaxw);
	}
	
	frame->tab_w=w;
	
	if(frame->bar_w==n*w){
		draw_frame_bar(frame, TRUE);
	}else{
		frame->bar_w=n*w;
		XResizeWindow(wglobal.dpy, frame->bar_win, frame->bar_w, frame->bar_h);
	}
}


/* 
 * Shade
 */


void frame_toggle_shade(WFrame *frame)
{
	set_frame_state(frame, frame->flags^WFRAME_SHADE);
}


void frame_toggle_sticky(WFrame *frame)
{
	int ws=WORKSPACE_STICKY;
	
	if(WWINOBJ_IS_STICKY(frame))
		ws=WORKSPACE_CURRENT;
	
	move_to_workspace((WWinObj*)frame, ws);
	
	/*draw_frame_bar(frame, TRUE);*/
}


/*
 * Decor toggle
 */

void frame_set_decor(WFrame *frame, int decorflags)
{
	int flagchg;
	int w, h, x, y;
	WClientWin *cwin;
	
	flagchg=(frame->flags&WFRAME_NO_DECOR)^decorflags;
	
	if(flagchg==0)
		return;
	
	if(flagchg&WFRAME_NO_BAR){
		if(frame->flags&WFRAME_NO_BAR){
			XMapWindow(wglobal.dpy, frame->bar_win);
			frame->bar_h=GRDATA->bar_height;
		}else{
			if(WFRAME_IS_SHADE(frame))
				set_frame_state(frame, frame->flags&~WFRAME_SHADE);
			XUnmapWindow(wglobal.dpy, frame->bar_win);
			frame->bar_h=0;
		}
	}

	if(flagchg&WFRAME_NO_BORDER){
		if(frame->flags&WFRAME_NO_BORDER){
			frame->frame_ix=frame->frame_iy=CF_BORDER_WIDTH;
		}else{
			frame->frame_ix=frame->frame_iy=0;
		}
	}

	frame->flags^=flagchg;
	
	w=frame->frame_iw+2*frame->frame_ix;
	h=frame->frame_ih+2*frame->frame_iy;
	frame->w=w;
	frame->h=h+frame->bar_h;
	
	XMoveResizeWindow(wglobal.dpy, frame->frame_win,
					  frame->x, frame->y+frame->bar_h, w, h);

	if(flagchg&WFRAME_NO_BAR && !(decorflags&WFRAME_NO_BAR))
		frame_recalc_bar(frame);
	
	cwin=first_clientwin((WThing*)frame);
	while(cwin!=NULL){
		x=frame->frame_ix;
		y=frame->frame_iy;
#ifndef CF_CWIN_TOPLEFT
		x+=(frame->frame_iw-cwin->client_w)/2;
		y+=(frame->frame_ih-cwin->client_h)/2;
#endif
		XMoveWindow(wglobal.dpy, cwin->client_win, x, y);
		cwin=next_clientwin(cwin);
	}
}
	

void frame_toggle_decor(WFrame *frame)
{
	int decorflags=0;
	
	if((frame->flags&WFRAME_NO_DECOR)==0)
		decorflags=WFRAME_NO_DECOR;
	
	frame_set_decor(frame, decorflags);
}

