/*
 * pwm/binding.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_BINDING_H
#define INCLUDED_BINDING_H

#include "common.h"
#include "function.h"

#define	ACTX_ROOT		0x00001
#define	ACTX_TAB		0x00010
#define	ACTX_CORNER		0x00020
#define ACTX_SIDE		0x00040
#define ACTX_MENU		0x00100
#define ACTX_MENUTITLE	0x00200
#define ACTX_WINDOW		0x01000
#define ACTX_DOCKWIN	0x02000
#define ACTX_GLOBAL		0x10000
#define ACTX_MOVERES	0x20000

#define ACTX_C_FRAME	(ACTX_TAB|ACTX_SIDE|ACTX_CORNER|ACTX_WINDOW)

#define ACT_KEYPRESS 		0
#define ACT_BUTTONPRESS		1
#define ACT_BUTTONMOTION	2
#define ACT_BUTTONCLICK		3
#define ACT_BUTTONDBLCLICK	4
#define ACT_N				5


typedef struct _WBinding{
	uint kcb;	/* keycode or button */
	uint state;
	uint actx;
	WFunction *func;
	WFuncArg arg;
} WBinding;


typedef struct _WBindmap{
	int nbindings;
	WBinding *bindings;
} WBindmap;


extern void init_bindings();
extern bool add_binding(uint act, uint actx, uint state, uint kcb,
						const WFuncBinder *binder);
extern void grab_bindings(Window win, uint actx);
extern WBinding *lookup_binding(uint act, uint actx, uint state, uint kcb);

extern bool add_pointer_binding(uint actx, uint state, uint kcb,
								const WFuncBinder *pressb,
								const WFuncBinder *clickb,
								const WFuncBinder *dblclickb,
								const WFuncBinder *motionb);

#endif /* INCLUDED_BINDING_H */
