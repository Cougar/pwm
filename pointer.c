/*
 * pwm/pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "pointer.h"
#include "frame.h"
#include "draw.h"
#include "moveres.h"
#include "cursor.h"
#include "event.h"
#include "menu.h"
#include "dock.h"
#include "binding.h"


/* */

static uint p_button=0;
static uint p_state=0, p_actx=0;
static int p_x=-1, p_y=-1;
static int p_orig_x=-1, p_orig_y=-1;
static bool p_motion=FALSE;
static int p_clickcnt=0;
static Time p_time=0;
static int p_moveres=0;
static int p_tab_x, p_tab_y;
static bool p_motiontmp_dirty;
static WBinding *p_motiontmp=NULL;
static WThing *p_dbltmp=NULL;


/* */


void pointer_change_context(WThing *thing, uint actx)
{
	wglobal.grab_holder=thing;
	p_actx=actx;
	p_motiontmp_dirty=TRUE;
}


void get_pointer_rootpos(int *xret, int *yret)
{
	Window root, win;
	int x, y, wx, wy;
	uint mask;
	
	XQueryPointer(wglobal.dpy, SCREEN->root, &root, &win,
				  xret, yret, &wx, &wy, &mask);
}


/* */


static bool time_in_treshold(Time time)
{
	Time t;
	
	if(time<p_time)
		t=p_time-time;
	else
		t=time-p_time;
	
	return t<wglobal.dblclick_delay;
}


static bool motion_in_treshold(int x, int y)
{
	return (x>p_x-CF_DRAG_TRESHOLD && x<p_x+CF_DRAG_TRESHOLD &&
			y>p_y-CF_DRAG_TRESHOLD && y<p_y+CF_DRAG_TRESHOLD);
}


bool find_window_at(int x, int y, Window *childret)
{
	int dstx, dsty;
	
	if(!XTranslateCoordinates(wglobal.dpy, SCREEN->root, SCREEN->root,
							  x, y, &dstx, &dsty, childret))
		return FALSE;
	
	if(*childret==None)
		return FALSE;
	
	return TRUE;
}


static void call_button(WBinding *binding, XButtonEvent *ev, bool nohand)
{
	WFuncClass *fclass;
	
	if(binding==NULL || binding->func==NULL || wglobal.grab_holder==NULL)
		return;
	
	fclass=binding->func->fclass;
	
	if(fclass->button==NULL){
		if(!nohand && fclass->handler!=NULL)
			fclass->handler(wglobal.grab_holder, binding->func, binding->arg);
		return;
	}

	fclass->button(wglobal.grab_holder, ev, binding->func, binding->arg);
}


/* */


static uint frame_press(WFrame *frame, XButtonEvent *ev, WThing **thing)
{
	int cw=CF_CORNER_SIZE, ch=CF_CORNER_SIZE;
	int actx=-1, tabnum=-1;
	bool ret;
	
	if(ev->window==frame->bar_win){
		actx=ACTX_TAB;
		/*if(frame->tab_w==0)
			tabnum=0;
		else*/
			tabnum=ev->x/frame->tab_w;
		p_tab_y=BAR_Y(frame);
		p_tab_x=BAR_X(frame)+tabnum*frame->tab_w;
		p_moveres|=MOVERES_TOP;
		*thing=nth_subthing((WThing*)frame, tabnum);
	}else if(ev->window==frame->frame_win){
		if(cw*2>FRAME_W(frame))
			cw=frame->w/2;

		if(ch*2>FRAME_H(frame))
			ch=FRAME_H(frame)/2;
			
		if(ev->x<cw)
			p_moveres|=MOVERES_LEFT;
		else if(ev->x>FRAME_W(frame)-cw)
			p_moveres|=MOVERES_RIGHT;

		if(ev->y<ch)
			p_moveres|=MOVERES_TOP;
		else if(ev->y>FRAME_H(frame)-ch)
			p_moveres|=MOVERES_BOTTOM;
	
		switch(p_moveres){
		case 0:
			if(ev->x<FRAME_W(frame)/3)
				p_moveres|=MOVERES_LEFT;
			else if(ev->x>FRAME_W(frame)*2/3)
				p_moveres|=MOVERES_RIGHT;
			
			if(ev->y<FRAME_H(frame)/2)
				p_moveres|=MOVERES_TOP;
			else if(ev->y>FRAME_H(frame)*2/3)
				p_moveres|=MOVERES_BOTTOM;
			
			actx=ACTX_WINDOW;
			break;
			
		case MOVERES_RIGHT:
		case MOVERES_LEFT:
		case MOVERES_TOP:
		case MOVERES_BOTTOM:
			actx=ACTX_SIDE;
			break;
			
		default:
			actx=ACTX_CORNER;
			break;
		}
	}else{
		actx=ACTX_WINDOW;
	}
	
	return actx;
}


static uint dock_press(WDock *dock, XButtonEvent *ev, WThing **thing)
{
	WClientWin *cwin=dockwin_at(dock, ev->x, ev->y);
	
	if(cwin==NULL){
		*thing=NULL;
		return 0;
	}
	
	*thing=(WThing*)cwin;
	
	return ACTX_DOCKWIN;
}


static uint menu_press(WMenu *menu, XButtonEvent *ev)
{
	if(ev->y<menu->title_height)
		return ACTX_MENUTITLE;
	return ACTX_MENU;
}


void handle_button_press(XButtonEvent *ev)
{
	WBinding *pressbind=NULL;
	WThing *thing=NULL;
	uint actx=0, state;
	uint button;
	
	p_moveres=0;
	p_motiontmp=NULL;
	
	state=ev->state;
	button=ev->button;
	
	thing=find_thing(ev->window);
	
	if(thing==NULL){
		/* root window? */
		if(ev->window!=ev->root)
			return;
		actx=ACTX_ROOT;
		assert(ev->root==SCREEN->root);
		thing=(WThing*)SCREEN;
	}else if(WTHING_IS(thing, WTHING_CLIENTWIN)){
		/* dock window? */
		if(WTHING_IS(thing, WTHING_DOCKWIN))
			actx=ACTX_DOCKWIN;
		else
			actx=ACTX_WINDOW;
	}else if(WTHING_IS(thing, WTHING_DOCK)){
		actx=dock_press(SCREEN->dock, ev, &thing);
	}else if(WTHING_IS(thing, WTHING_FRAME)){
		actx=frame_press((WFrame*)thing, ev, &thing);
	}else if(WTHING_IS(thing, WTHING_MENU)){
		actx=menu_press((WMenu*)thing, ev);
	}

	wglobal.grab_holder=thing;
	
	if(!(actx&ACTX_MENU) && wglobal.input_mode==INPUT_CTXMENU)
		destroy_contextual_menus();

	if(p_clickcnt==1 && time_in_treshold(ev->time) && p_button==button &&
	   p_state==state && wglobal.grab_holder==p_dbltmp && actx==p_actx){
		pressbind=lookup_binding(ACT_BUTTONDBLCLICK, actx, state, button);
	}
	
	if(pressbind==NULL)
		pressbind=lookup_binding(ACT_BUTTONPRESS, actx, state, button);
	
end:
	p_dbltmp=thing;
	p_actx=actx;
	p_button=button;
	p_state=state;
	p_orig_x=p_x=ev->x_root;
	p_orig_y=p_y=ev->y_root;
	p_time=ev->time;
	p_motion=FALSE;
	p_clickcnt=0;
	p_motiontmp_dirty=TRUE;
	
	call_button(pressbind, ev, FALSE);
}


bool handle_button_release(XButtonEvent *ev)
{
	WBinding *binding=NULL;
	
	if(p_button!=ev->button)
	   	return FALSE;

	if(p_motion==FALSE){
		p_clickcnt=1;
		binding=lookup_binding(ACT_BUTTONCLICK, p_actx, p_state, p_button);
	}else if(p_motiontmp_dirty){
		binding=lookup_binding(ACT_BUTTONMOTION, p_actx, p_state, p_button);
	}else{
		binding=p_motiontmp;
	}

	call_button(binding,  ev, !p_clickcnt);
	
	return TRUE;
}


void handle_pointer_motion(XMotionEvent *ev)
{
	WFuncClass *fclass;
	WThing *tmp;
	int dx, dy;
	
	if(p_motion==FALSE && motion_in_treshold(ev->x_root, ev->y_root))
		return;
	
	if(p_motiontmp_dirty){
		p_motiontmp=lookup_binding(ACT_BUTTONMOTION, p_actx, p_state, p_button);
		p_motiontmp_dirty=FALSE;
	}

	p_time=ev->time;
	dx=ev->x_root-p_x;
	dy=ev->y_root-p_y;
	p_x=ev->x_root;
	p_y=ev->y_root;	

	if(p_motiontmp==NULL || wglobal.grab_holder==NULL)
		return;

	fclass=p_motiontmp->func->fclass;
	if(fclass->motion!=NULL){
		fclass->motion(wglobal.grab_holder, ev, dx, dy,
					   p_motiontmp->func, p_motiontmp->arg);
	}
	
	p_motion=TRUE;
}


/* */


static void drag_tab(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	int x, y;
	WScreen *scr=SCREEN;
	
	if(p_motion==FALSE){
		if(!WTHING_IS(wglobal.grab_holder, WTHING_CLIENTWIN))
			return;
		
		change_grab_cursor(CURSOR_DRAG);
		
		XResizeWindow(wglobal.dpy, scr->tabdrag_win,
					  frame->tab_w, frame->bar_h);
		XMapRaised(wglobal.dpy, scr->tabdrag_win);
		((WClientWin*)wglobal.grab_holder)->flags|=CWIN_DRAG;
		draw_frame_bar(frame, FALSE);
	}
	
	if(((WClientWin*)wglobal.grab_holder)==NULL)
		return;
	
	x=p_tab_x+(p_x-p_orig_x);
	y=p_tab_y+(p_y-p_orig_y);
	
	XMoveWindow(wglobal.dpy, scr->tabdrag_win, x, y);
	
	if(p_motion==FALSE)
		draw_tabdrag(((WClientWin*)wglobal.grab_holder));
}	


static void drag_tab_end(WFrame *frame, XButtonEvent *ev)
{
	Window win;
	WFrame *newframe=NULL;
	WScreen *scr=SCREEN;
	bool is_new=FALSE;
	int x, y;
	
	if(wglobal.grab_holder==NULL ||
	   !WTHING_IS(wglobal.grab_holder, WTHING_CLIENTWIN))
		return;
	
	XUnmapWindow(wglobal.dpy, scr->tabdrag_win);	
	((WClientWin*)wglobal.grab_holder)->flags&=~CWIN_DRAG;
	draw_frame_bar(frame, TRUE);

	if(find_window_at(ev->x_root, ev->y_root, &win))
		newframe=find_frame_of(win);

	x=p_tab_x+(p_x-p_orig_x);
	y=p_tab_y+(p_y-p_orig_y);

	attachdetach_clientwin(newframe, (WClientWin*)wglobal.grab_holder, TRUE, x, y);
}


/* */


void drag_handler(WThing *thing, XMotionEvent *ev, int dx, int dy,
				  WFunction *func, WFuncArg arg)
{
	WWinObj *obj;
	int stepsize=1;
	int moveres_mode=0;
	
	obj=winobj_of(thing);

	if(obj==NULL)
		return;

	switch(func->opval){
	case DRAG_MOVE_STEPPED:
		stepsize=CF_STEP_SIZE;
	case DRAG_MOVE:
		if(p_motion==FALSE)
			change_grab_cursor(CURSOR_MOVE);
		
		move_winobj(obj, dx, dy, stepsize);
		return;
	}
	
	if(!WTHING_IS(obj, WTHING_FRAME))
		return;

	switch(func->opval){
	case DRAG_RESIZE_STEPPED:
		stepsize=CF_STEP_SIZE;
	case DRAG_RESIZE:
		if(p_motion==FALSE)
			change_grab_cursor(CURSOR_RESIZE);

		moveres_mode=p_moveres;
		if(moveres_mode&MOVERES_LEFT)
			dx=-dx;
		if(moveres_mode&MOVERES_TOP)
			dy=-dy;
	
		resize_frame((WFrame*)obj, dx, dy, moveres_mode, stepsize);
		break;
		
	case DRAG_TAB:
		drag_tab((WFrame*)obj, ev, dx, dy);
		break;
	}
}
			

void drag_end(WThing *thing, XButtonEvent *ev, WFunction *func, WFuncArg arg)
{
	WWinObj *obj;
	
	obj=winobj_of(thing);
	
	if(obj==NULL)
		return;
	
	switch(func->opval){
	case DRAG_MOVE_STEPPED:
	case DRAG_MOVE:
		move_winobj_end(obj);
		break;
		
	case DRAG_RESIZE_STEPPED:
	case DRAG_RESIZE:
		if(WTHING_IS(obj, WTHING_FRAME))
			resize_frame_end((WFrame*)obj);
		break;
		
	case DRAG_TAB:
		if(WTHING_IS(obj, WTHING_FRAME))
			drag_tab_end((WFrame*)obj, ev);
	}
}
