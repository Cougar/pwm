/*
 * pwm/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>

#include "common.h"
#include "placement.h"
#include "screen.h"
#include "frame.h"
#include "pointer.h"


/* Random bitmasks */
#define RMASK1 0x100
#define RMASK2 0x200
#define RMASK3 0x400


/* random_placement - place randomly in given box
 */
static void random_placement(int mx, int my, int *xret, int *yret)
{
	*xret+=(mx<=0 ? 0 : rand()%mx);
	*yret+=(my<=0 ? 0 : rand()%my);
}


/* smart_placement - try to find a place where the frame will not
 * obscure given rectangle [x, x+pw)x[y, y+pw)
 */
static bool smart_placement(int x, int y, int pw, int ph,
							int w, int h, int *xret, int *yret)
{
	int hs1, vs1, hs2, vs2;
	int rnum=rand();
	
	hs1=x;
	vs1=y;
	hs2=SCREEN->width-x-pw;
	vs2=SCREEN->height-y-ph;
	x=0;
	y=0;
	
	if(hs2>=w){
		/* Randomly select between left/right side if there is space
		 * on both sides.
		 */
		if(hs1<w || rnum&RMASK1){
			x=hs1+pw;
			hs1=hs2;
		}
	}

	if(vs2>=h){
		/* Randomly select between top/bottom if there is space in
		 * both directions.
		 */
		if(vs1<h || rnum&RMASK2){
			y=vs1+ph;
			vs1=vs2;
		}
	}
	
	if(hs1>=w){
		/* Randomly select between horizontal and vertical if there is
		 * space in both directions.
		 */
		if(vs1>=h && rnum&RMASK3)
			goto vplace;
		/* Place on left/right side */
		y=0;
		vs1=SCREEN->height;
	}else if(vs1>=h){
vplace:
		/* Place on top/bottom side */
		x=0;
		hs1=SCREEN->width;
	}else{
		return FALSE;
	}

	/* An area has been found where the frame will fit: Place it randomly
	 * inside that rectangle.
	 */
	*xret=x; *yret=y;
	random_placement(hs1-w, vs1-h, xret, yret);
	
	return TRUE;
}


/* */


static WWinObj* is_occupied(int x, int y, int w, int h, int ws)
{
	WWinObj *current;
	int i;
	int px,py,pw,ph;
	
	current=(WWinObj*)subthing((WThing*)SCREEN, WTHING_WINOBJ);

	for(current=(WWinObj*)subthing((WThing*)SCREEN, WTHING_WINOBJ);
		current!=NULL;
		current=(WWinObj*)next_thing((WThing*)current, WTHING_WINOBJ)){
		
		if(current->workspace!=ws && current->workspace!=WORKSPACE_STICKY)
			continue;
		
		if(!WWINOBJ_IS_MAPPED(current))
			continue;
		
		if(WTHING_IS(current, WTHING_FRAME) && WFRAME_IS_SHADE(current)){
			px=BAR_X((WFrame*)current);
			py=BAR_Y((WFrame*)current);
			pw=BAR_W((WFrame*)current);
			ph=BAR_H((WFrame*)current);
		}else{
			px=current->x;
			py=current->y;
			pw=current->w;
			ph=current->h;
		}
		
		if(x>=px+pw)
			continue;
		if(y>=py+ph)
			continue;
		if(x+w<=px)
			continue;
		if(y+h<=py)
			continue;
		return current;
	}
	
	return NULL;
}


#ifdef CF_FLACCID_PLACEMENT_UDLR

static int next_least_x(int x, int ws)
{
	WWinObj* p;
	int px, pw;
	int retx=SCREEN->width;
	
	for(p=(WWinObj*)subthing((WThing*)SCREEN, WTHING_WINOBJ);
		p!=NULL;
		p=(WWinObj*)next_thing((WThing*)p, WTHING_WINOBJ)){
			
		if(WTHING_IS(p, WTHING_FRAME) && WFRAME_IS_SHADE(p)){
			px=BAR_X((WFrame*)p);
			pw=BAR_W((WFrame*)p);
		}else{
			px=p->x;
			pw=p->w;
		}
		
		if(px+pw>x && px+pw<retx)
			retx=px+pw;
   }
	
	return retx+1;
}

#else

static int next_lowest_y(int y, int ws)
{
	WWinObj* p;
	int py, ph;
	int rety=SCREEN->height;
	
    for(p=(WWinObj*)subthing((WThing*)SCREEN, WTHING_WINOBJ);
		p!=NULL;
        p=(WWinObj*)next_thing((WThing*)p, WTHING_WINOBJ)){
					
		if(WTHING_IS(p, WTHING_FRAME) && WFRAME_IS_SHADE(p)){
			py=BAR_Y((WFrame*)p);
			ph=BAR_H((WFrame*)p);
		}else{
			py=p->y;
			ph=p->h;
		}
		
		if(py+ph>y && py+ph<rety)
			rety=py+ph;
	}
	
	return rety+1;
}

#endif


/* */


static bool flaccid_placement(int w, int h, int ws, int *xret, int *yret)
{
	WWinObj* p;
	int i=0;
	int x=0, y=0;
	
#ifdef CF_FLACCID_PLACEMENT_UDLR
	while(x<SCREEN->width){
		p=is_occupied(x, y, w, h, ws);
		while(p!=NULL && y+h<SCREEN->height){
			y=p->y+1;
			y+=(WTHING_IS(p, WTHING_FRAME) && WFRAME_IS_SHADE(p)) ?
				BAR_H((WFrame*)p) : p->h;
			p=is_occupied(x, y, w, h, ws);
		}
		if(y+h<SCREEN->height && x+w<SCREEN->width){
			*xret=x;
			*yret=y;
			return TRUE;
		}else{
			x=next_least_x(x, ws);
			y=0;
		}
	}
#else
	while(y<SCREEN->height){
		p=is_occupied(x, y, w, h, ws);
		while(p!=NULL && x+w<SCREEN->width){
			x=p->x+1;
			x+=(WTHING_IS(p, WTHING_FRAME) && WFRAME_IS_SHADE(p)) ?
				BAR_W((WFrame*)p) : p->w;
			p=is_occupied(x, y, w, h, ws);
		}
		if(x+w<SCREEN->width && y+h<SCREEN->height){
			*xret=x;
			*yret=y;
			return TRUE;
		}else{
			y=next_lowest_y(y, ws);
			x=0;
		}
	}
#endif	

	*xret=0; *yret=0;
	return FALSE;

}


/* */


void calc_placement(int w, int h, int ws, int *xret, int *yret)
{
	WWinObj *current=wglobal.current_winobj;
	int x, y;

	if(ws<0)
		ws=SCREEN->current_workspace;
	
	if(flaccid_placement(w, h, ws, xret, yret))
		return;
	
	if(current!=NULL){
		/* Try to place it so that it doesn't overlap current winobj. */
		if(smart_placement(current->x, current->y, current->w, current->h,
						   w, h, xret, yret))
			return;
        
		/* Try to place it so that it is not under a small box around
		 * the cursor and thus will not take the focus.
		 */
		get_pointer_rootpos(&x, &y);
		if(smart_placement(x-CF_DRAG_TRESHOLD, y-CF_DRAG_TRESHOLD, 
						   CF_DRAG_TRESHOLD, CF_DRAG_TRESHOLD,
						   w, h, xret, yret))
			return;
		
		/* Fall back to random placement... */
	}
	
	*xret=0; *yret=0;
	random_placement(SCREEN->width-w, SCREEN->height-h, xret, yret);
}

