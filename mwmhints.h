/*
 * pwm/mwmhints.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_MWMHINTS_H
#define INCLUDED_MWMHINTS_H

#include <X11/Xmd.h>

#include "common.h"


#define MWM_HINTS_FUNCTIONS		0x0001
#define MWM_HINTS_DECORATIONS	0x0002
#define MWM_HINTS_INPUT_MODE	0x0004
#define MWM_HINTS_INPUT_STATUS	0x0008

#define MWM_FUNC_ALL			0x0001
#define MWM_FUNC_RESIZE			0x0002
#define MWM_FUNC_MOVE			0x0004
#define MWM_FUNC_ICONIFY		0x0008
#define MWM_FUNC_MAXIMIZE		0x0010
#define MWM_FUNC_CLOSE			0x0020

#define MWM_DECOR_ALL			0x0001
#define MWM_DECOR_BORDER		0x0002
#define MWM_DECOR_HANDLE		0x0004
#define MWM_DECOR_TITLE			0x0008
#define MWM_DECOR_MENU			0x0010
#define MWM_DECOR_ICONIFY		0x0020
#define MWM_DECOR_MAXIMIZE		0x0040

#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 	2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3

typedef struct _WMwmHints{
	CARD32 flags;
	CARD32 functions;
	CARD32 decorations;
	INT32 inputmode;
	CARD32 status;
} WMwmHints;

#define MWM_DECOR_NDX 3
#define MWM_N_HINTS	5


/* */


extern void get_mwm_hints(Window win, int *flags);

#endif /* INCLUDED_MWMHINTS_H */
