/*
 * pwm/thing.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_THING_H
#define INCLUDED_THING_H

#include "common.h"

#define WTHING_UNKNOWN		0x0000
#define	WTHING_SCREEN		0x0100
#define WTHING_WORKSPACE	0x0200

#define	WTHING_CLIENTWIN	0x0400
#define WTHING_DOCKWIN		0x0420

#define	WTHING_WINOBJ		0x0800
#define	WTHING_FRAME		0x0810
#define WTHING_MENU			0x0820
#define WTHING_DOCK			0x0840

#define WTHING_IS(THING, TYPE) (((THING)->type&(TYPE))==(TYPE))

#define WTHING_SUBDEST		0x10000
#define WTHING_UNFOCUSABLE 	0x20000
#define WTHING_IS_UNFOCUSABLE(F) ((F)->flags&WTHING_UNFOCUSABLE)


#define INHERIT_WTHING                     \
	int type, flags;                       \
	struct _WThing *t_parent, *t_children; \
 	struct _WThing *t_next, *t_prev

#define WTHING_INIT(OBJ, TYPE)         \
	(OBJ)->type=(TYPE);                \
	(OBJ)->flags=0;                    \
	(OBJ)->t_parent=(OBJ)->t_children= \
	(OBJ)->t_next=(OBJ)->t_prev=NULL

typedef struct _WThing{
	INHERIT_WTHING;
} WThing;


extern void link_thing(WThing *parent, WThing *child);
extern void link_thing_before(WThing *before, WThing *child);
extern void unlink_thing(WThing *thing);
extern void destroy_subthings(WThing *thing);
extern void destroy_thing(WThing *thing);
extern void free_thing(WThing *thing);

WThing *next_thing(WThing *first, int filt);
WThing *prev_thing(WThing *first, int filt);
WThing *subthing(WThing *parent, int filt);

struct _WClientWin *next_clientwin(struct _WClientWin *first);
struct _WClientWin *prev_clientwin(struct _WClientWin *first);
struct _WClientWin *first_clientwin(WThing *parent);

extern struct _WThing *nth_thing(WThing *first, int num);
extern struct _WThing *nth_subthing(WThing *parent, int num);

extern struct _WThing *find_thing(Window win);
extern struct _WThing *find_thing_t(Window win, int type);
extern struct _WFrame *find_frame_of(Window win);
extern struct _WWinObj *find_winobj_of(Window win);
extern struct _WClientWin *find_clientwin(Window win);
extern struct _WWinObj *winobj_of(WThing *thing);

#endif /* INCLUDED_THING_H */
