/*
 * pwm/winprops.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_WINPROPS_H
#define INCLUDED_WINPROPS_H

#include "common.h"
#include "winobj.h"

enum{
	WILDMODE_APP=0,
	WILDMODE_YES=1,
	WILDMODE_NO=2
};
	
typedef struct _WWinProp {
	char *data;
	char *wclass;
	char *winstance;
	
	int init_frame;
	int init_workspace;	
	int dockpos;
	int wildmode;
	
	struct _WWinProp *next, *prev;
} WWinProp;

extern WWinProp *find_winprop(const char *wclass, const char *winstance);
extern WWinProp *find_winprop_win(Window win);
extern void free_winprop(WWinProp *winprop);
extern void register_winprop(WWinProp *winprop);

#endif /* INCLUDED_WINPROPS_H */
