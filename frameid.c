/*
 * pwm/frameid.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include <limits.h>

#include "common.h"
#include "frameid.h"
#include "frame.h"
#include "thing.h"
#include "property.h"


static int frame_id_cntr=FRAME_ID_START_CLIENT;
static int n_frames=0;


static void reorder_frame_ids()
{
	WFrame *frame;
	WClientWin *cwin;
	
	frame_id_cntr=FRAME_ID_START_CLIENT;
	
	for(frame=(WFrame*)subthing((WThing*)SCREEN, WTHING_FRAME);
		frame!=NULL;
		frame=(WFrame*)next_thing((WThing*)frame, WTHING_FRAME)){
		
		if(frame->frame_id<FRAME_ID_START_CLIENT)
			continue;
	
		frame->frame_id=++frame_id_cntr;

		cwin=first_clientwin((WThing*)frame);
		while(cwin!=NULL){
			set_integer_property(cwin->client_win, wglobal.atom_frame_id,
								 frame->frame_id);
			cwin=next_clientwin(cwin);
		}
	}
}


/* */


int new_frame_id()
{	
	if(((frame_id_cntr-FRAME_ID_START_CLIENT)&FRAME_ID_REORDER_INTERVAL)==
	   FRAME_ID_REORDER_INTERVAL-1 || frame_id_cntr==INT_MAX)
		reorder_frame_ids();

	if(frame_id_cntr==INT_MAX){
		die("You seem to have a _lot_ of frames on your screen and there"
			"are no free frame IDs (frame_id_cntr==INT_MAX)."
			"Refusing to continue. Thou can not always win."
			"Sorry.");
	}
	
	n_frames++;
	
	return ++frame_id_cntr;
}


int use_frame_id(int id)
{
	if(id>frame_id_cntr)
		frame_id_cntr=id;
	
	n_frames++;

	return id;
}
	   
	   
void free_frame_id(int id)
{
	if(frame_id_cntr==INT_MAX)
		reorder_frame_ids();
	
	n_frames--;
}


WFrame *find_frame_by_id(int id)
{
	WFrame *frame;
	
	if(id==0)
		return NULL;
	
	for(frame=(WFrame*)subthing((WThing*)SCREEN, WTHING_FRAME);
		frame!=NULL;
		frame=(WFrame*)next_thing((WThing*)frame, WTHING_FRAME)){
		
		if(frame->frame_id==id)
			return frame;
	}

	return NULL;
}

