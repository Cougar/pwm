/*
 * pwm/winlist.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include <string.h>

#include "common.h"
#include "menu.h"
#include "screen.h"
#include "frame.h"
#include "clientwin.h"
#include "focus.h"
#include "winobj.h"
#include "frameid.h"


static bool winlist_menudata_init(WMenuData *data, WThing *context);
static void winlist_menudata_exec(WMenuEnt *entry, WThing *context);
static void winlist_menudata_deinit(WMenuData *data);
static void attachlist_menudata_exec(WMenuEnt *entry, WThing *context);
static void detachlist_menudata_exec(WMenuEnt *entry, WThing *context);
static void goto_clientwin(WClientWin *cwin);
static bool wslist_menudata_init(WMenuData *data, WThing *context);
static void wslist_menudata_exec(WMenuEnt *entry, WThing *context);
static void movetows_menudata_exec(WMenuEnt *entry, WThing *context);


/* */


static WMenuData winlist_menudata={
	"Go to", "goto_menu", 0,
	winlist_menudata_init,
	winlist_menudata_exec,
	winlist_menudata_deinit,
	0, NULL,
	0, NULL,
	NULL, NULL
};


static WMenuData attachlist_menudata={
	"Attach", "attach_menu", WMENUDATA_CONTEXTUAL,
	winlist_menudata_init,
	attachlist_menudata_exec,
	winlist_menudata_deinit,
	0, NULL,
	0, NULL,
	NULL, NULL
};


static WMenuData detachlist_menudata={
	"Detach", "detach_menu", 0,
	winlist_menudata_init,
	detachlist_menudata_exec,
	winlist_menudata_deinit,
	0, NULL,
	0, NULL,
	NULL, NULL
};


static WMenuData wslist_menudata={
	"Go to WS", "ws_menu", 0,
	wslist_menudata_init,
	wslist_menudata_exec,
	winlist_menudata_deinit,
	0, NULL,
	0, NULL,
	NULL, NULL
};


static WMenuData movetows_menudata={
	"Move to WS", "movetows_menu", WMENUDATA_CONTEXTUAL,
	wslist_menudata_init,
	movetows_menudata_exec,
	winlist_menudata_deinit,
	0, NULL,
	0, NULL,
	NULL, NULL
};

	
/* */


void register_winlist()
{
	register_menudata(&winlist_menudata);
	register_menudata(&attachlist_menudata);
	register_menudata(&detachlist_menudata);
	register_menudata(&wslist_menudata);
	register_menudata(&movetows_menudata);
}


void update_winlist()
{
	if(winlist_menudata.nref!=0){
		winlist_menudata_deinit(&winlist_menudata);
		winlist_menudata_init(&winlist_menudata, NULL);
		update_menus(&winlist_menudata);
	}

	if(detachlist_menudata.nref!=0){
		winlist_menudata_deinit(&detachlist_menudata);
		winlist_menudata_init(&detachlist_menudata, NULL);
		update_menus(&detachlist_menudata);
	}
	/* Attachlist is not updated as it is contextual and update should
	 * not occur when the menu is open.
	 */
}

	
/* */

static void do_free_winlist(WMenuEnt *ents, int n)
{
	int i;
	
	for(i=0; i<n; i++){
		if(ents[i].name!=NULL)
			free(ents[i].name);
	}
	
	free(ents);
}


static int menuent_cmp(const void *e1, const void *e2)
{
	WMenuEnt *ent1=(WMenuEnt*)e1;
	WMenuEnt *ent2=(WMenuEnt*)e2;
	WFrame *f1=CWIN_FRAME(ent1->u.winlist.clientwin);
	WFrame *f2=CWIN_FRAME(ent2->u.winlist.clientwin);
	
	if(f1->workspace<f2->workspace)
		return -1;
	if(f1->workspace>f2->workspace)
		return 1;
	
	if(f1->frame_id<f2->frame_id)
		return -1;
	if(f1->frame_id>f2->frame_id)
		return 1;
	
	if(ent1->u.winlist.sort<ent2->u.winlist.sort)
		return -1;
	if(ent1->u.winlist.sort>ent2->u.winlist.sort)
		return 1;
	   
	return 0;
}
	
		
static bool winlist_menudata_init(WMenuData *data, WThing *context)
{
	WMenuEnt *ents;
	WFrame *frame;
	int ws;
	WClientWin *cwin;
	int nents, length, entryname_length, i=0;
	const char *winname;
	char *entryname;
	
	nents=SCREEN->n_clientwin;
	
	/* Some extra space will be allocated depending on number of dockapps */
	ents=ALLOC_N(WMenuEnt, nents);
	
	if(ents==NULL){
		warn_err();
		return FALSE;
	}
	
	for(frame=(WFrame*)subthing((WThing*)SCREEN, WTHING_FRAME);
		frame!=NULL;
		frame=(WFrame*)next_thing((WThing*)frame, WTHING_FRAME)){
		
		ws=workspace_of((WWinObj*)frame);
		
		/* Don't list active frame */
		if(data==&attachlist_menudata){
			if(context!=NULL && winobj_of(context)==(WWinObj*)frame)
				continue;
		}
	
		for(cwin=first_clientwin((WThing*)frame);
			cwin!=NULL;
			cwin=next_clientwin(cwin)){
				  
			if(!CWIN_HAS_FRAME(cwin))
				continue;
			   
			winname=clientwin_full_label(cwin);
			length=32+strlen(winname);
			
			entryname=ALLOC_N(char, length);
			
			if(entryname==NULL){
				warn_err();
				do_free_winlist(ents, i);
				return FALSE;
			}
			
			if(ws>=0){
				entryname_length=snprintf(entryname+0, length, "%d%c", ws+1,
										  (cwin==frame->current_cwin
										   ? '+' : '-'));
			}else{
				entryname_length=snprintf(entryname+0, length, "%c%c",
										  (ws==WORKSPACE_STICKY ? '*' : '?'),
										  (cwin==frame->current_cwin
										   ? '+' : '-'));
			}

			snprintf(entryname + entryname_length, length - entryname_length,
					 "  %s", winname);
			
			ents[i].name=entryname;
			ents[i].flags=0;
			ents[i].u.winlist.clientwin=cwin;
			ents[i].u.winlist.sort=i;
			i++;
		}
	}
	
	nents=i;
	
	qsort(ents, nents, sizeof(WMenuEnt), menuent_cmp);
	
	data->entries=ents;
	data->nentries=nents;
	
	return TRUE;
}


static void winlist_menudata_deinit(WMenuData *data)
{
	do_free_winlist(data->entries, data->nentries);
	data->entries=NULL;
	data->nentries=0;
}
					

static void winlist_menudata_exec(WMenuEnt *entry, WThing *context)
{
	if(entry->u.winlist.clientwin!=NULL)
		goto_clientwin(entry->u.winlist.clientwin);	
}


static void attachlist_menudata_exec(WMenuEnt *entry, WThing *context)
{
	WWinObj *obj=winobj_of(context);
	
	if(!WTHING_IS(obj, WTHING_FRAME))
		return;

	if(entry->u.winlist.clientwin==NULL)
		return;

	attachdetach_clientwin((WFrame*)obj, entry->u.winlist.clientwin,
						   FALSE, 0, 0);
}


static void detachlist_menudata_exec(WMenuEnt *entry, WThing *context)
{
	WFrame *frame;
	WClientWin *cwin=entry->u.winlist.clientwin;
	
	if(cwin==NULL)
		return;
	
	attachdetach_clientwin(NULL, cwin, FALSE, 0, 0);
	goto_clientwin(cwin);
}


/* */


static void goto_winobj(WWinObj *obj)
{
	int ws, x, y;
	
	ws=workspace_of(obj);

	if(ws==WORKSPACE_UNKNOWN)
		move_to_workspace(obj, WORKSPACE_CURRENT);
	else if(ws!=WORKSPACE_STICKY)
		switch_workspace_num(ws);

	if(!winobj_is_visible(obj)){
		x=obj->x%SCREEN->width;
		y=obj->y%SCREEN->height;
		set_winobj_pos(obj, x, y);
	}

	raise_winobj(obj);
}


static void goto_clientwin(WClientWin *cwin)
{
	WFrame *frame;
	int ws, x, y;
	
	if(!CWIN_HAS_FRAME(cwin))
		return;
	
	frame=CWIN_FRAME(cwin);
	
	goto_winobj((WWinObj*)frame);
	
	if(frame->current_cwin!=cwin)
		frame_switch_clientwin(frame, cwin);
	
	if(!WFRAME_IS_NORMAL(frame))
		set_frame_state(frame, frame->flags&~(WFRAME_HIDDEN|WFRAME_SHADE));	
	
	set_focus((WThing*)cwin);
}


void goto_frame(int id)
{
	WFrame *frame;
	
	frame=find_frame_by_id(id);
	
	if(frame!=NULL){
		goto_winobj((WWinObj*)frame);

		if(!WFRAME_IS_NORMAL(frame))
			set_frame_state(frame, frame->flags&~(WFRAME_HIDDEN|WFRAME_SHADE));	
		
		if(frame->current_cwin!=NULL)
			set_focus((WThing*)frame->current_cwin);
	}
}


/*{{{ Workspace list */


static bool wslist_menudata_init(WMenuData *data, WThing *context)
{
	WMenuEnt *ents;
	int nents, l, i=0;
	char *entryname;
	
	nents=SCREEN->n_workspaces;
	
	/* Some extra space will be allocated depending on number of dockapps */
	ents=ALLOC_N(WMenuEnt, nents);
	
	if(ents==NULL){
		warn_err();
		return FALSE;
	}

	l=sizeof(int)*3+4+1; /* should be enough */
	
	for(i=0; i<nents; i++){
		
		entryname=ALLOC_N(char, l);
		
		if(entryname==NULL){
			warn_err();
			do_free_winlist(ents, i);
			return FALSE;
		}
		

		snprintf(entryname, l, "ws %d", i+1);
		
		ents[i].name=entryname;
		ents[i].flags=0;
		ents[i].u.num=i;
		SET_INT_ARG(ents[i].u.f.arg, i);
	}
	
	data->entries=ents;
	data->nentries=nents;
	
	return TRUE;
}


static void wslist_menudata_exec(WMenuEnt *entry, WThing *context)
{
	switch_workspace_num(entry->u.num);
}


static void movetows_menudata_exec(WMenuEnt *entry, WThing *context)
{
	WWinObj *obj;
	
	if(context!=NULL){
		obj=winobj_of(context);
	
		if(obj!=NULL)
			move_to_workspace(obj, entry->u.num);
	}
}

/*}}}*/
