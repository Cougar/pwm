/*
 * pwm/winobj.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_WINOBJ_H
#define INCLUDED_WINOBJ_H

#include "common.h"
#include "thing.h"


#define WWINOBJ_MAPPED 0x100000
#define WWINOBJ_HIDDEN 0x200000

#define WORKSPACE_STICKY -2
#define WORKSPACE_CURRENT -1
#define WORKSPACE_UNKNOWN -1

#define WWINOBJ_IS_MAPPED(F) ((F)->flags&WWINOBJ_MAPPED)
#define WWINOBJ_IS_HIDDEN(F) ((F)->flags&WWINOBJ_HIDDEN)
#define WWINOBJ_IS_STICKY(F) ((F)->workspace==WORKSPACE_STICKY)


enum{
	LVL_KEEP_ON_BOTTOM=0,
	LVL_NORMAL=1,
	LVL_KEEP_ON_TOP=2,
	LVL_MENU=3,
	N_STACK_LVLS=4,
	LVL_OTHER=-1 /* used for transients and submenus that are stacked
				  * above their "parents" 
				  */
};


/* */


#define INHERIT_WWINOBJ                         \
	INHERIT_WTHING;                             \
                                                \
	int stack_lvl;                              \
	struct _WWinObj *stack_prev, *stack_next;   \
	struct _WWinObj *stack_above;               \
	struct _WWinObj *stack_above_list;          \
	int workspace;                              \
	int x, y, w, h


/* */


typedef struct _WWinObj{
	INHERIT_WWINOBJ;
} WWinObj;


/* */


extern void add_winobj(WWinObj *obj, int ws, int init_stack_lvl);
extern void add_winobj_above(WWinObj *obj, WWinObj *above);

extern void restack_winobj(WWinObj *obj, int stack_lvl, bool raiselower);
extern void restack_winobj_above(WWinObj *obj, WWinObj *above, bool raiselower);

extern void unlink_winobj_d(WWinObj *obj);

extern void raise_winobj(WWinObj *obj);
extern void lower_winobj(WWinObj *obj);
extern void raiselower_winobj(WWinObj *obj);

extern void map_winobj(WWinObj *obj);
extern void unmap_winobj(WWinObj *obj);
extern void do_map_winobj(WWinObj *obj);
extern void do_unmap_winobj(WWinObj *obj);

extern WWinObj *traverse_winobjs(WWinObj *current, WWinObj *root);
extern WWinObj *traverse_winobjs_b(WWinObj *p, WWinObj *root, WWinObj **next);
extern WWinObj *init_traverse_winobjs_b(WWinObj *root, WWinObj **next);

extern void set_winobj_pos(WWinObj *obj, int x, int y);
extern bool winobj_is_visible(WWinObj *obj);

#endif /* INCLUDED_WINOBJ_H */

