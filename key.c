/*
 * pwm/key.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include "common.h"
#include "key.h"
#include "binding.h"


void handle_keypress(XKeyEvent *ev)
{
	WThing *thing=NULL;
	WBinding *binding=NULL;
	WFunction *func;
	uint actx=0;
	
	thing=wglobal.grab_holder;
	
	if(wglobal.input_mode==INPUT_MOVERES){
		actx=ACTX_MOVERES;
	}else{
		if(thing==NULL){
			thing=find_thing(ev->subwindow);
			
			if(thing==NULL)
				thing=find_thing(ev->window);
			
			if(thing==NULL)
				thing=(WThing*)SCREEN;
		}
			
		if(WTHING_IS(thing, WTHING_MENU))
			actx=ACTX_MENU;
		
		if(wglobal.input_mode==INPUT_NORMAL){
			actx|=ACTX_GLOBAL;
			
			if(WTHING_IS(thing, WTHING_FRAME))
				actx|=ACTX_C_FRAME;
			else if(WTHING_IS(thing, WTHING_CLIENTWIN))
				actx|=ACTX_WINDOW;
			else if(WTHING_IS(thing, WTHING_DOCKWIN))
				actx|=ACTX_DOCKWIN;
		}
	}

	binding=lookup_binding(ACT_KEYPRESS, actx, ev->state, ev->keycode);

	if(binding!=NULL && binding->func!=NULL){
		func=binding->func;
		if(func->fclass->handler!=NULL)
			func->fclass->handler(thing, func, binding->arg);
	}
}
