/*
 * pwm/binding.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include <string.h>

#ifndef CF_NO_LOCK_HACK
#define CF_HACK_IGNORE_EVIL_LOCKS
#endif

#ifdef CF_HACK_IGNORE_EVIL_LOCKS
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#endif

#include "common.h"
#include "event.h"
#include "binding.h"


/* */


static WBindmap bindmaps[ACT_N]={
	{0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}
};


/* */


#ifdef CF_HACK_IGNORE_EVIL_LOCKS

#define N_EVILLOCKS 3
#define N_LOOKUPEVIL 2
#define N_MODS 8

static uint evillockmasks[N_EVILLOCKS]={
	 0, 0, LockMask
};

static const KeySym evillocks[N_LOOKUPEVIL]={
	XK_Num_Lock, XK_Scroll_Lock
};

static const uint modmasks[N_MODS]={
	ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask,
	Mod4Mask, Mod5Mask
};

static uint evilignoremask=LockMask;

static void lookup_evil_locks();

static void evil_grab_key(Display *display, uint keycode, uint modifiers,
						  Window grab_window, bool owner_events,
						  int pointer_mode, int keyboard_mode);

static void evil_grab_button(Display *display, uint button, uint modifiers,
							 Window grab_window, bool owner_events,
							 uint event_mask, int pointer_mode,
							 int keyboard_mode, Window confine_to,
							 Cursor cursor);

#endif


bool add_binding(uint act, uint actx, uint state, uint kcb,
				 const WFuncBinder *binder)
{
	WBinding *binding;
	WBindmap *bindmap;
	int i;
	
	if(binder->func==NULL || act>=ACT_N)
		return FALSE;
	
	bindmap=&(bindmaps[act]);
	
	binding=REALLOC_N(bindmap->bindings, WBinding, bindmap->nbindings,
					  bindmap->nbindings+1);
	
	if(binding==NULL){
		warn_err();
		return FALSE;
	}
	
	bindmap->bindings=binding;
	
	if(act==ACT_KEYPRESS)
		kcb=XKeysymToKeycode(wglobal.dpy, kcb);
	
	for(i=0; i<bindmap->nbindings; i++){
		if(binding[i].kcb>=kcb)
			break;
	}
	
	for(; i<bindmap->nbindings; i++){
		if(binding[i].kcb>kcb)
			break;
		if(binding[i].state>state)
			break;
	}
	
	memmove(&(binding[i+1]), &(binding[i]),
			sizeof(WBinding)*(bindmap->nbindings-i));
	
	bindmap->nbindings++;
	
	binding=&(binding[i]);
	
	binding->kcb=kcb;
	binding->state=state;
	binding->actx=actx;
	binding->func=binder->func;
	binding->arg=binder->arg;
	
	return TRUE;
}


bool add_pointer_binding(uint actx, uint state, uint button,
						 const WFuncBinder *pressb,
						 const WFuncBinder *clickb,
						 const WFuncBinder *dblclickb,
						 const WFuncBinder *motionb)
{
	int failed=0;
	
	if(pressb!=NULL && pressb->func!=NULL)
		failed+=!add_binding(ACT_BUTTONPRESS, actx, state, button, pressb);
	
	if(clickb!=NULL && clickb->func!=NULL)
		failed+=!add_binding(ACT_BUTTONCLICK, actx, state, button, clickb);
	
	if(dblclickb!=NULL && dblclickb->func!=NULL)
		failed+=!add_binding(ACT_BUTTONDBLCLICK, actx, state, button, dblclickb);
	
	if(motionb!=NULL && motionb->func!=NULL)
		failed+=!add_binding(ACT_BUTTONMOTION, actx, state, button, motionb);
	
	return failed==0;
}


void init_bindings()
{
	/* Maybe set a few default bindings? */
#ifdef CF_HACK_IGNORE_EVIL_LOCKS
	lookup_evil_locks();
#endif
}


/* */


static void do_grab_keys(WBindmap *bindmap, Window win, uint actx)
{
	WBinding *binding=bindmap->bindings;
	int i;
	
	for(i=0; i<bindmap->nbindings; i++, binding++){
		if(!(binding->actx&actx))
			continue;
		
#ifndef CF_HACK_IGNORE_EVIL_LOCKS			
		XGrabKey(wglobal.dpy, binding->kcb, binding->state, win, True,
				 GrabModeAsync, GrabModeAsync);
#else			
		evil_grab_key(wglobal.dpy, binding->kcb, binding->state, win, True,
					  GrabModeAsync, GrabModeAsync);
#endif			
	}
}


static void do_grab_buttons(WBindmap *bindmap, Window win, uint actx)
{
	WBinding *binding=bindmap->bindings;
	int i;
	
	for(i=0; i<bindmap->nbindings; i++, binding++){
		if(!(binding->actx&actx))
			continue;
		
		/* Don't grab buttons with no modifiers */
		if(binding->state==0)
			continue;
		
#ifndef CF_HACK_IGNORE_EVIL_LOCKS			
		XGrabButton(wglobal.dpy, binding->kcb, binding->state, win, True,
					GRAB_POINTER_MASK, GrabModeAsync, GrabModeAsync,
					None, None);
#else			
		evil_grab_button(wglobal.dpy, binding->kcb, binding->state, win, True,
						 GRAB_POINTER_MASK, GrabModeAsync, GrabModeAsync,
						 None, None);
#endif
	}
}


void grab_bindings(Window win, uint actx)
{
	int i;
	
	do_grab_keys(&(bindmaps[ACT_KEYPRESS]), win, actx);
	
	for(i=1; i<ACT_N; i++)
		do_grab_buttons(&(bindmaps[i]), win, actx);
}


/* */


WBinding *lookup_binding(uint act, uint actx, uint state, uint kcb)
{
	WBindmap *bindmap=&(bindmaps[act]);
	WBinding *binding;
	int i;
	
#ifdef CF_HACK_IGNORE_EVIL_LOCKS
	state&=~evilignoremask;
#endif
again:
	binding=bindmap->bindings;
	
	for(i=0; i<bindmap->nbindings; binding++, i++){
		if(binding->kcb>=kcb)
			break;
	}
	
	for(; i<bindmap->nbindings; binding++, i++){
		if(binding->kcb>kcb)
			break;
		if(binding->state>state)
			break;
		if(binding->state<state)
			continue;
		if(binding->actx&actx)
			return binding;
	}
	
	if(act==ACT_KEYPRESS){
		if(kcb==AnyKey)
			return NULL;
		kcb=AnyKey;
	}else{
		if(kcb==AnyButton)
			return NULL;
		kcb=AnyButton;
	}
	
	goto again;
}


/*
 * A dirty hack to deal with (==ignore) evil locking modifier keys.
 */

#ifdef CF_HACK_IGNORE_EVIL_LOCKS

static void lookup_evil_locks()
{
	XModifierKeymap *modmap;
	uint keycodes[N_LOOKUPEVIL];
	int i, j;
	
	for(i=0; i<N_LOOKUPEVIL; i++)
		keycodes[i]=XKeysymToKeycode(wglobal.dpy, evillocks[i]);
	
	modmap=XGetModifierMapping(wglobal.dpy);
	
	if(modmap==NULL)
		return;
	
	for(j=0; j<N_MODS*modmap->max_keypermod; j++){
		for(i=0; i<N_LOOKUPEVIL; i++){
			if(keycodes[i]==None)
				continue;
			if(modmap->modifiermap[j]==keycodes[i]){
				evillockmasks[i]=modmasks[j/modmap->max_keypermod];
				evilignoremask|=evillockmasks[i];
			}
		}
	}
	
	XFreeModifiermap(modmap);
}


static void evil_grab_key(Display *display, uint keycode, uint modifiers,
						  Window grab_window, bool owner_events,
						  int pointer_mode, int keyboard_mode)
{
	uint mods;
	int i, j;
	
	XGrabKey(display, keycode, modifiers, grab_window, owner_events,
			 pointer_mode, keyboard_mode);
	
	for(i=0; i<N_EVILLOCKS; i++){
		if(evillockmasks[i]==0)
			continue;
		mods=modifiers;
		for(j=i; j<N_EVILLOCKS; j++){
			if(evillockmasks[j]==0)
				continue;			
			mods|=evillockmasks[j];			
			XGrabKey(display, keycode, mods,
					 grab_window, owner_events, pointer_mode, keyboard_mode);
			if(i==j)
				continue;
			XGrabKey(display, keycode,
					 modifiers|evillockmasks[i]|evillockmasks[j],
					 grab_window, owner_events, pointer_mode, keyboard_mode);
		}
	}	
}


static void evil_grab_button(Display *display, uint button, uint modifiers,
							 Window grab_window, bool owner_events,
							 uint event_mask, int pointer_mode,
							 int keyboard_mode, Window confine_to,
							 Cursor cursor)
{
	uint mods;
	int i, j;
	
	XGrabButton(display, button, modifiers,
				grab_window, owner_events, event_mask, pointer_mode,
				keyboard_mode, confine_to, cursor);
	
	for(i=0; i<N_EVILLOCKS; i++){
		if(evillockmasks[i]==0)
			continue;
		mods=modifiers;
		for(j=i; j<N_EVILLOCKS; j++){			
			if(evillockmasks[j]==0)
				continue;			
			mods|=evillockmasks[j];			
			XGrabButton(display, button, mods,
						grab_window, owner_events, event_mask, pointer_mode,
						keyboard_mode, confine_to, cursor);
			if(i==j)
				continue;
			XGrabButton(display, button,
						modifiers|evillockmasks[i]|evillockmasks[j],
						grab_window, owner_events, event_mask, pointer_mode,
						keyboard_mode, confine_to, cursor);
		}			
	}
}

#endif /* CF_HACK_IGNORE_EVIL_LOCKS */
