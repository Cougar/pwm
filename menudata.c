/*
 * pwm/menudata.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include <string.h>

#include "common.h"
#include "menu.h"


static WMenuData *menudata_list=NULL;


static void user_menudata_exec_func(WMenuEnt *entry, WThing *context);


/* */


bool register_menudata(WMenuData *menudata)
{
	LINK_ITEM(menudata_list, menudata, menudata_next, menudata_prev);
	
	if(menudata->flags&WMENUDATA_USER)
		menudata->exec_func=user_menudata_exec_func;

	return TRUE;
}


void unregister_menudata(WMenuData *menudata)
{
	if(menudata->menudata_prev!=NULL){
		UNLINK_ITEM(menudata_list, menudata, menudata_next, menudata_prev);
	}
}


WMenuData *lookup_menudata(const char *name)
{
	WMenuData *p=menudata_list;
	
	if(name==NULL)
		return NULL;
	
	for(; p!=NULL; p=p->menudata_next){
		if(p->name==NULL)
			continue;
		if(strcmp(p->name, name)==0)
			return p;
	}
	
	return NULL;
}


/* */


void free_user_menudata(WMenuData *menudata)
{
	int i;
	WMenuEnt *entry;
	
	unregister_menudata(menudata);
	
	if(menudata->title!=NULL)
		free(menudata->title);

	entry=menudata->entries;
	
	for(i=0; i<menudata->nentries; i++, entry++){
		if(entry->name!=NULL)
			free(entry->name);
		
		if(entry->u.f.arg_type==ARGTYPE_STRING)
			free(ARG_TO_STRING(entry->u.f.arg));
	}
	
	if(menudata->entries!=NULL){
		free(menudata->entries);
		menudata->entries=NULL;
	}
}


static void user_menudata_exec_func(WMenuEnt *entry, WThing *context)
{
	WFuncClass *fclass;
	
	if(entry->u.f.function==NULL)
		return;
	
	fclass=entry->u.f.function->fclass;
	
	if(fclass==NULL || fclass->handler==NULL)
		return;
	
	fclass->handler(context, entry->u.f.function, entry->u.f.arg);
}
