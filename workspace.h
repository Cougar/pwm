/*
 * pwm/workspace.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_WORKSPACE_H
#define INCLUDED_WORKSPACE_H

#include "common.h"
#include "thing.h"

struct _WWinObj;

extern void switch_workspace_num(int num);
extern void switch_workspace_hrot(int dir);
extern void switch_workspace_vrot(int dir);
extern void init_workspaces();

extern void move_to_workspace(WWinObj *obj, int ws);
extern bool on_current_workspace(WWinObj *obj);
extern int workspace_of(WWinObj *obj);
extern void dodo_switch_workspace(int num);
						
#endif /* INCLUDED_WORKSPACE_H */
