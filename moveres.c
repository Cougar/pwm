/*
 * pwm/moveres.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include "common.h"
#include "moveres.h"
#include "frame.h"
#include "menu.h"
#include "draw.h"
#include "focus.h"


/* */


#define OP_MOVE 1
#define OP_RESIZE 2

static int tmp_x=0, tmp_y=0, tmp_w=0, tmp_h=0;
static int tmp_dw=0, tmp_dh=0;
static int tmp_dx=0, tmp_dy=0;
static int opflag=0;
static char moveres_tmpstr[CF_MAX_MOVERES_STR_SIZE];

static XSizeHints dummy_size_hints={
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1,
	{0,0},
	{0,0},
	0,0,0
};


/* */


static void show_pos(int x, int y)
{
	sprintf(moveres_tmpstr, "%+d %+d", x, y);
	
	draw_moveres(moveres_tmpstr);
}


static void show_size(int w, int h, XSizeHints *hints)
{
	if(hints->flags&PResizeInc){
		w/=hints->width_inc;
		h/=hints->height_inc;
	}
		
	sprintf(moveres_tmpstr, "%dx%d", w, h);
	
	draw_moveres(moveres_tmpstr);
}


static void beg_moveres(WWinObj *obj, bool grab)
{
	tmp_x=obj->x;
	tmp_y=obj->y;
	tmp_dw=tmp_w=obj->w;
	tmp_dh=tmp_h=obj->h;
	tmp_dx=tmp_dy=0;
	
	XMapRaised(wglobal.dpy, SCREEN->moveres_win);
	
	if(grab)
		XGrabServer(wglobal.dpy);
}


static void end_moveres(bool ungrab)
{
	if(ungrab)
		XUngrabServer(wglobal.dpy);
	
	XUnmapWindow(wglobal.dpy, SCREEN->moveres_win);
}


/* */


static void do_check_snap(int w, int h, int stepsize, int *rdx, int *rdy)
{
	int sw, sh;
	int tx, ty;
	int dx, dy;
	int tmp;

	sw=SCREEN->width;
	sh=SCREEN->height;
	
	dx=*rdx;
	dy=*rdy;
	tx=dx+w;
	ty=dy+h;
	
	if(dx<0 && dx>-CF_EDGE_RESISTANCE){
		tmp_dx=dx;
		dx=0;
	}else if(tx>sw && tx<sw+CF_EDGE_RESISTANCE){
		tmp_dx=tx-sw;
		dx=sw-w;
	}else{
		tmp_dx=0;
	}

	if(dy<0 && dy>-CF_EDGE_RESISTANCE){
		tmp_dy=dy;
		dy=0;
	}else if(ty>sh && ty<sh+CF_EDGE_RESISTANCE){
		tmp_dy=ty-sh;
		dy=sh-h;
	}else{
		tmp_dy=0;
	}
	
	if(stepsize>1){
		if((tmp=dx%stepsize)){
			tmp_dx+=tmp;
			dx-=tmp;
		}
	
		if((tmp=dy%stepsize)){
			tmp_dy+=tmp;
			dy-=tmp;
		}
	}
	
	*rdx=dx;
	*rdy=dy;
}


static void check_snap(WWinObj *obj, int stepsize, int *rdx, int *rdy)
{
	do_check_snap(tmp_w, tmp_h, stepsize, rdx, rdy);
}


/* */


#define ALIGN_UP(X, A) ((((X)+((A)-1))/(A))*(A))
#define ALIGN_DOWN(X, A) (((X)/(A))*(A))


static void do_correct_aspect(int max_w, int max_h, int ax, int ay,
							  int *wret, int *hret)
{
	int w=*wret, h=*hret;

	if(ax>ay){
		h=(w*ay)/ax;
		if(max_h>0 && h>max_h){
			h=max_h;
			w=(h*ax)/ay;
		}
	}else{
		w=(h*ax)/ay;
		if(max_w>0 && w>max_w){
			w=max_w;
			h=(w*ay)/ax;
		}
	}
	
	*wret=w;
	*hret=h;
}


void correct_aspect(int max_w, int max_h, XSizeHints *hints,
					int *wret, int *hret)
{
	if(!(hints->flags&PAspect))
		return;
	
	if(*wret*hints->max_aspect.y>*hret*hints->max_aspect.x){
		do_correct_aspect(max_w, max_h,
						  hints->min_aspect.x, hints->min_aspect.y,
						  wret, hret);
	}

	if(*wret*hints->min_aspect.y<*hret*hints->min_aspect.x){
		do_correct_aspect(max_w, max_h,
						  hints->max_aspect.x, hints->max_aspect.y,
						  wret, hret);
	}
}


/* cwin==NULL  fmode
 * 	FALSE		FALSE	honour clientwin's size hints
 *  FALSE		TRUE	honour frame's size hints
 *  TRUE		-		ignore clientwin's size hints :-)
 */
static void do_calc_size(WFrame *frame, XSizeHints *hints, bool fmode,
						 int winc, int hinc, int *wr, int *hr)
{
	int w=*wr, h=*hr;
	
	/* Check min-max range */
	if(!fmode && hints->flags&PMinSize){
		if(w<hints->min_width)
			w=hints->min_width;
		if(h<hints->min_height)
			h=hints->min_height;
	}else{
		if(w<frame->min_w)
			w=frame->min_w;
		if(h<frame->min_h)
			h=frame->min_h;
	}
		
	
	if(!fmode && hints->flags&PMaxSize){
		if(hints->max_width<w)
			w=hints->max_width;
		if(hints->max_height<h)
			h=hints->max_height;
	}else{
		if(!FRAME_MAXW_UNSET(frame) && frame->max_w<w)
			w=frame->max_w;
		if(!FRAME_MAXH_UNSET(frame) && frame->max_h<h)
			h=frame->max_h;
	}
		

	/* Correct aspect ratio */
	if(!fmode)
		correct_aspect(w, h, hints, &w, &h);

	
	/* Check resize increment */
	if(hints->flags&PResizeInc){
		if(winc==0)
			winc=hints->width_inc;
		else if(winc<=hints->width_inc && hints->width_inc!=0)
			winc=hints->width_inc;
		else
			winc=ALIGN_UP(winc, hints->width_inc);
		
		if(hinc==0)
			hinc=hints->height_inc;
		else if(hinc<=hints->height_inc && hints->height_inc!=0)
			hinc=hints->height_inc;
		else
			hinc=ALIGN_UP(hinc, hints->height_inc);
	}
	
	if(winc>1)
		w=hints->base_width+ALIGN_DOWN(w-hints->base_width, winc);
		
	if(hinc>1)
		h=hints->base_height+ALIGN_DOWN(h-hints->base_height, hinc);
	
	*wr=w;
	*hr=h;
}


void calc_size(WFrame *frame, WClientWin *cwin, bool fmode, int *wr, int *hr)
{

	XSizeHints *hints=&dummy_size_hints;
	
	if(cwin!=NULL)
		hints=&(cwin->size_hints);
	
	do_calc_size(frame, hints, fmode, 0, 0, wr, hr);
}

			   
/* 
 * XOR-resize
 */

static void resize_xor(WFrame *frame, int dx, int dy, int mode, int stepsize)
{
	XSizeHints *hints=&dummy_size_hints;
	int x, y, w, h;
	
	if(frame->current_cwin!=NULL)
		hints=&(frame->current_cwin->size_hints);
	
	if(opflag==0)
		beg_moveres((WWinObj*)frame, TRUE);
	
	w=tmp_w;
	h=tmp_h;
	x=tmp_x;
	y=tmp_y;
	
	if(mode&RESIZE_WDEC)
		dx=-dx;
	if(mode&RESIZE_HDEC)
		dy=-dy;

	/* Calc new size */
	if((mode&(MOVERES_RIGHT|MOVERES_LEFT))!=0){
		tmp_dw+=dx;
		w=tmp_dw;
	}

	if((mode&(MOVERES_TOP|MOVERES_BOTTOM))!=0){
		tmp_dh+=dy;
		h=tmp_dh;
	}
	
	w-=frame->frame_ix*2;
	h-=frame->frame_iy*2+frame->bar_h;
	
	do_calc_size(frame, hints, frame->cwin_count!=1,
				 stepsize, stepsize, &w, &h);
	
	w+=frame->frame_ix*2;
	h+=frame->frame_iy*2+frame->bar_h;

	/* Move? */
	if(mode&MOVERES_LEFT){
		dx=w-tmp_w;
		if(mode&MOVERES_RIGHT)
			dx/=2;
		x-=dx;
	}

	if(mode&MOVERES_TOP){
		dy=h-tmp_h;
		if(mode&MOVERES_BOTTOM)
			dy/=2;
		y-=dy;
	}
	

	if(h!=tmp_h || w!=tmp_w || opflag==0){
		draw_rubberband((WWinObj*)frame, x, y, w, h);
		if(opflag!=0)
			draw_rubberband((WWinObj*)frame, tmp_x, tmp_y, tmp_w, tmp_h);
	}
	
	tmp_w=w; tmp_h=h;
	tmp_x=x; tmp_y=y;

	w-=frame->frame_ix*2;
	h-=frame->frame_iy*2+frame->bar_h;
	show_size(w, h, hints);
	
	opflag|=OP_RESIZE;
}


/*
 * XOR-move
 */

static void move_xor(WWinObj *obj, int dx, int dy, int stepsize)
{
	if(opflag==0)
		beg_moveres(obj, TRUE);
	
	dx+=tmp_x+tmp_dx;
	dy+=tmp_y+tmp_dy;
	
	check_snap(obj, stepsize, &dx, &dy);

	draw_rubberband(obj, dx, dy, tmp_w, tmp_h);

	if(opflag!=0)
		draw_rubberband(obj, tmp_x, tmp_y, tmp_w, tmp_h);
	
	tmp_x=dx;
	tmp_y=dy;
	
	show_pos(dx, dy);
	opflag|=OP_MOVE;
}


/*
 * End-function for XOR-resize and move
 */

static void xor_end(WWinObj *obj)
{
	WFrame *frame;
	
	draw_rubberband(obj, tmp_x, tmp_y, tmp_w, tmp_h);
	
	end_moveres(TRUE);

	set_winobj_pos(obj, tmp_x, tmp_y);
	if(opflag&OP_RESIZE && WTHING_IS(obj, WTHING_FRAME)){
		frame=(WFrame*)obj;
		tmp_w-=frame->frame_ix*2;
		tmp_h-=frame->frame_iy*2+frame->bar_h;
		set_frame_size(frame, tmp_w, tmp_h);
	}
	opflag=0;
}


/*
 * Cancel-function for XOR-resize and move
 */

static void xor_cancel(WWinObj *obj)
{
	draw_rubberband(obj, tmp_x, tmp_y, tmp_w, tmp_h);
	
	end_moveres(TRUE);
	opflag=0;
}


/*
 * Opaque-move
 */

static void move_opaque(WWinObj *obj, int dx, int dy, int stepsize)
{
	if(opflag==0)
		beg_moveres(obj, FALSE);

	tmp_x+=dx+tmp_dx;
	tmp_y+=dy+tmp_dy;
	
	check_snap(obj, stepsize, &tmp_x, &tmp_y);
		
	set_winobj_pos(obj, tmp_x, tmp_y);
	show_pos(tmp_x, tmp_y);
	opflag|=OP_MOVE;
}


static void move_opaque_end(WWinObj *obj)
{
	end_moveres(FALSE);
	opflag=0;
}


/* */


static bool can_opaque_move(WWinObj *obj)
{
	int ss, fs;
	
	if((WTHING_IS(obj, WTHING_FRAME) && WFRAME_IS_SHADE(obj))
	   || SCREEN->opaque_move>=100)
		return TRUE;
	
	fs=obj->w*obj->h;
	ss=SCREEN->width*SCREEN->height;
	return (fs*100/ss)<SCREEN->opaque_move;
}


void move_winobj(WWinObj *obj, int dx, int dy, int stepsize)
{
	if(can_opaque_move(obj))
		move_opaque(obj, dx, dy, stepsize);
	else
		move_xor(obj, dx, dy, stepsize);
}


void move_winobj_end(WWinObj *obj)
{
	if(can_opaque_move(obj))
		move_opaque_end(obj);
	else
		xor_end(obj);	
}


void resize_frame(WFrame *frame, int dx, int dy, int mode, int stepsize)
{
	resize_xor(frame, dx, dy, mode, stepsize);
}


void resize_frame_end(WFrame *frame)
{
	xor_end((WWinObj*)frame);
}


/* 
 * Keyboard moveres-mode
 */


static void do_keyboard_move(WWinObj *obj, int mask, int stepsize)
{
	int dx=0, dy=0;
	
	wglobal.input_mode=INPUT_MOVERES;
	wglobal.grab_holder=(WThing*)obj;

	if(mask&MOVERES_RIGHT)
		dx=stepsize;
	else if(mask&MOVERES_LEFT)
		dx=-stepsize;
	if(mask&MOVERES_BOTTOM)
		dy=stepsize;
	else if(mask&MOVERES_TOP)
		dy=-stepsize;
	
	move_xor(obj, dx, dy, 0);
}


void keyboard_move(WWinObj *obj, int mask)
{
	do_keyboard_move(obj, mask, 1);
}


void keyboard_move_stepped(WWinObj *obj, int mask)
{
	do_keyboard_move(obj, mask, CF_STEP_SIZE);
}


static void do_keyboard_resize(WFrame *frame, int mask, int stepsize)
{
	int dx, dy;

	dx=dy=(stepsize==0 ? 1 : stepsize);

	wglobal.input_mode=INPUT_MOVERES;
	wglobal.grab_holder=(WThing*)frame;

	resize_xor(frame, dx, dy, mask, stepsize);
}


void keyboard_resize(WFrame *frame, int mask)
{
	do_keyboard_resize(frame, mask, 0);
}


void keyboard_resize_stepped(WFrame *frame, int mask)
{
	do_keyboard_resize(frame, mask, CF_STEP_SIZE);
}


void keyboard_moveres_end(WWinObj *obj)
{
	if(opflag!=0)
		xor_end(obj);
	wglobal.input_mode=INPUT_NORMAL;
}


void keyboard_moveres_cancel(WWinObj *obj)
{
	if(opflag!=0)
		xor_cancel(obj);
	wglobal.input_mode=INPUT_NORMAL;
}


void keyboard_moveres_begin(WWinObj *obj)
{
	if(wglobal.input_mode!=INPUT_MOVERES){
		wglobal.input_mode=INPUT_MOVERES;
		wglobal.grab_holder=(WThing*)obj;
		move_xor(obj, 0, 0, 0);
	}
}


/* */


#ifdef CF_PACK_MOVE

void pack_move(WWinObj *obj, int mask)
{
	WWinObj *temp_obj=obj;
	int new_x;
	int new_y;
	
#ifdef DEBUG
	fprintf(stderr, "pack_move with %d\n", mask);
#endif /* DEBUG */   
	new_x=obj->x;
	new_y=obj->y;
	
	if (mask==MOVERES_RIGHT) {
		new_x=SCREEN->width;
		while(1){
			temp_obj=(WWinObj*)nth_thing((WThing*)temp_obj, 1);
			if(temp_obj==obj)
				break;
			if(temp_obj->workspace!=obj->workspace)
				continue;
			if(temp_obj->x < new_x &&
			   temp_obj->x > obj->x + obj->w &&
			   temp_obj->y < obj->y + obj->h &&
			   temp_obj->y + temp_obj->h >= obj->y){
				new_x=temp_obj->x;
			}
		}
		new_x-=obj->w;
	}
	else if(mask==MOVERES_LEFT) {
		new_x=0;
		while(1){
			temp_obj=(WWinObj*)nth_thing((WThing*)temp_obj, 1);
			if(temp_obj==obj)
				break;
			if(temp_obj->workspace!=obj->workspace)
				continue;
			if(temp_obj->x + temp_obj->w > new_x &&
				temp_obj->x + temp_obj->w < obj->x &&
				temp_obj->y < obj->y + obj->h &&
				temp_obj->y + temp_obj->h >= obj->y){
				new_x=temp_obj->x + temp_obj->w;
			}
		}
	}
	else if(mask==MOVERES_BOTTOM) {
		new_y=SCREEN->height;
		while(1){
			temp_obj=(WWinObj*)nth_thing((WThing*)temp_obj, 1);
			if(temp_obj==obj)
				break;
			if(temp_obj->workspace!=obj->workspace)
				continue;
			if(temp_obj->y < new_y &&
			   temp_obj->y > obj->y + obj->h &&
			   temp_obj->x < obj->x + obj->w &&
			   temp_obj->x + temp_obj->w > obj->x){
				new_y=temp_obj->y;
			}
		}
		new_y-=obj->h;
	}
	else if(mask==MOVERES_TOP) {
		new_y=0;
		while(1){
			temp_obj=(WWinObj*)nth_thing((WThing*)temp_obj, 1);
			if(temp_obj==obj)
				break;
			if(temp_obj->workspace!=obj->workspace)
				continue;
			if(temp_obj->y + temp_obj->h > new_y &&
			   temp_obj->y + temp_obj->h < obj->y &&
			   temp_obj->x < obj->x + obj->w &&
			   temp_obj->x + temp_obj->w > obj->x){
				new_y=temp_obj->y + temp_obj->h;
			}
		}
	}
#ifdef DEBUG
	fprintf(stderr,
			"Moving... Frame: %dx%d, %dx%d\nNew pos: %dx%d\n", 
			obj->x, obj->y, obj->w, obj->h,
			new_x, new_y);
#endif /* DEBUG */
	
	if(WTHING_IS(obj, WTHING_FRAME))
		set_frame_pos((WFrame*)obj, new_x, new_y);
	else if(WTHING_IS(obj, WTHING_MENU))
		set_menu_pos((WMenu*)obj, new_x, new_y);
}

#endif /* CF_PACKMOVE */


#ifdef CF_GOTODIR

void gotodir(WWinObj *obj, int mask)
{
	WWinObj *temp_obj=obj, *tmp=NULL;
	int new_x;
	int new_y;
	
	new_x=obj->x;
	new_y=obj->y;
	
	if(mask==MOVERES_RIGHT){
		new_x=SCREEN->width;
		while(1){
			temp_obj=(WWinObj*)nth_thing((WThing*)temp_obj, 1);
			if(temp_obj==obj)
				break;
			if(temp_obj->workspace!=obj->workspace)
				continue;
			if(temp_obj->x < new_x &&
			   temp_obj->x > obj->x &&
			   temp_obj->y <= obj->y + obj->h &&
			   temp_obj->y + temp_obj->h >= obj->y){
				new_x=temp_obj->x;
				tmp=temp_obj;
			}
		}
	}
	else if(mask==MOVERES_LEFT){
		new_x=0;
		while(1){
			temp_obj=(WWinObj*)nth_thing((WThing*)temp_obj, 1);
			if(temp_obj==obj)
				break;
			if(temp_obj->workspace!=obj->workspace)
				continue;
			if(temp_obj->x + temp_obj->w > new_x &&
			   temp_obj->x <= obj->x &&
			   temp_obj->y <= obj->y + obj->h &&
			   temp_obj->y + temp_obj->h >= obj->y){
				new_x=temp_obj->x + temp_obj->w;
				tmp=temp_obj;
			}
		}
	}
	else if(mask==MOVERES_BOTTOM){
		new_y=SCREEN->height;
		while(1){
			temp_obj=(WWinObj*)nth_thing((WThing*)temp_obj, 1);
			if(temp_obj==obj)
				break;
			if(temp_obj->workspace!=obj->workspace)
				continue;
			if(temp_obj->y < new_y &&
			   temp_obj->y >= obj->y &&
			   temp_obj->x <= obj->x + obj->w &&
			   temp_obj->x + temp_obj->w > obj->x){
				new_y=temp_obj->y;
				tmp=temp_obj;
			}
		}
	}
	else if(mask==MOVERES_TOP){
		new_y=0;
		while(1){
			temp_obj=(WWinObj*)nth_thing((WThing*)temp_obj, 1);
			if(temp_obj==obj)
				break;
			if(temp_obj->workspace!=obj->workspace)
				continue;
			if(temp_obj->y + temp_obj->h > new_y &&
			   temp_obj->y <= obj->y &&
			   temp_obj->x <= obj->x + obj->w &&
			   temp_obj->x + temp_obj->w > obj->x){
				new_y=temp_obj->y + temp_obj->h;
				tmp=temp_obj;
			}
		}
	}
	
	if(tmp!=NULL)
		set_focus((WThing*)tmp);
}

#endif /* CF_GOTODIR */

