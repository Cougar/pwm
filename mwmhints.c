/*
 * pwm/mwmhints.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "property.h"
#include "mwmhints.h"
#include "frame.h"


#ifndef CF_NO_WILD_WINDOWS
void get_mwm_hints(Window win, int *flags)
{
	WMwmHints *hints;
	int n;
	
	n=do_get_property(wglobal.dpy, win, wglobal.atom_mwm_hints,
					  wglobal.atom_mwm_hints, MWM_N_HINTS, (uchar**)&hints);
	
	if(n<MWM_DECOR_NDX)
		return;
	
	if(hints->flags&MWM_HINTS_DECORATIONS &&
	   (hints->decorations&MWM_DECOR_ALL)==0){
		*flags|=CWIN_WILD;
		
		if(hints->decorations&MWM_DECOR_BORDER ||
		   hints->decorations&MWM_DECOR_TITLE)
			*flags&=~CWIN_WILD;
	}
	
	XFree((void*)hints);
}
#endif
