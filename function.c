/*
 * pwm/function.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "function.h"
#include "exec.h"
#include "frame.h"
#include "clientwin.h"
#include "focus.h"
#include "workspace.h"
#include "menu.h"
#include "pointer.h"
#include "moveres.h"
#include "winlist.h"


/* */


static WFuncHandler winobj_handler;
static WFuncHandler winobj_i_handler;
static WFuncHandler frame_handler;
static WFuncHandler frame_i_handler;
static WFuncHandler clientframe_handler;
static WFuncHandler clientframe_i_handler;
static WFuncHandler cwin_handler;
static WFuncHandler global_handler;
static WFuncHandler global_i_handler;
static WFuncHandler global_s_handler;
static WFuncHandler menu_i_handler;

static WFuncHandler show_menu_handler;
static WButtonHandler show_menu_but_handler;

extern WButtonHandler drag_end;
extern WMotionHandler drag_handler;

static void wrap_switch_clientwin(WClientWin *cwin);
static void wrap_set_stack_lvl(WFrame *frame, int lvl);


/* */


static WFuncClass fclass_frame={
	frame_handler, NULL, NULL, ARGTYPE_NONE
};

static WFuncClass fclass_frame_i={
	frame_i_handler, NULL, NULL, ARGTYPE_INT
};

static WFuncClass fclass_clientframe={
	frame_handler, NULL, NULL, ARGTYPE_NONE
};

static WFuncClass fclass_clientframe_i={
	frame_i_handler, NULL, NULL, ARGTYPE_INT
};

static WFuncClass fclass_winobj={
	winobj_handler, NULL, NULL, ARGTYPE_NONE
};

static WFuncClass fclass_winobj_i={
	winobj_i_handler, NULL, NULL, ARGTYPE_INT
};

static WFuncClass fclass_global={
	global_handler, NULL, NULL, ARGTYPE_NONE
};

static WFuncClass fclass_global_i={
	global_i_handler, NULL, NULL, ARGTYPE_INT
};

static WFuncClass fclass_global_s={
	global_s_handler, NULL, NULL, ARGTYPE_STRING
};

static WFuncClass fclass_cwin={
	cwin_handler, NULL, NULL, ARGTYPE_NONE
};

static WFuncClass fclass_drag={
	NULL, drag_handler, drag_end, ARGTYPE_NONE
};

static WFuncClass fclass_show_menu={
	show_menu_handler, NULL, show_menu_but_handler, ARGTYPE_STRING
};

static WFuncClass fclass_menu_cmd={
	menu_i_handler, NULL, NULL, ARGTYPE_NONE
};


/* */

#define FN(CLASS, NAME, FUNC) {&fclass_##CLASS, NAME, (void*)FUNC, 0}
#define FNI(CLASS, NAME, ARG) {&fclass_##CLASS, NAME, NULL, ARG}

extern void debug_winobj(WWinObj *obj);

static WFunction funcs[]={
	/* frame */
	FN(frame, 		"toggle_shade", 	frame_toggle_shade),
	FN(frame,		"toggle_stick",		frame_toggle_sticky),
	FN(frame, 		"toggle_decor", 	frame_toggle_decor),
	FN(frame,		"attach_tagged",	frame_attach_tagged),
	FN(frame_i, 	"toggle_maximize", 	frame_toggle_maximize),
	FN(frame_i,		"switch_nth",		frame_switch_nth),
	FN(frame_i, 	"switch_rot",		frame_switch_rot),
	FN(frame_i,		"set_stack_lvl",	wrap_set_stack_lvl),
	
	/* winobj */
	FN(winobj, 		"raise", 			raise_winobj),
	FN(winobj, 		"lower", 			lower_winobj),
	FN(winobj, 		"raiselower", 		raiselower_winobj),
	FN(winobj,		"kb_moveres",		keyboard_moveres_begin),
	FN(winobj_i, 	"move_to_ws", 		move_to_workspace),
	
	/* cwin */
	FN(cwin,		"close",			close_clientwin),
	FN(cwin,		"kill",				kill_clientwin),
	FN(cwin,		"toggle_tagged",	clientwin_toggle_tagged),
	FN(cwin,		"detach",			clientwin_detach),

	/* global */
	FN(global_i,	"circulate",		circulate),
	FN(global_i,	"circulateraise",	circulateraise),
	FN(global_i,	"switch_ws_num",	switch_workspace_num),
	FN(global_i,	"switch_ws_hrot",	switch_workspace_hrot),
	FN(global_i,	"switch_ws_vrot",	switch_workspace_vrot),
	FN(global,		"join_tagged",		join_tagged),
	FN(global_s,	"restart_other",	wm_restart_other),
	FN(global,		"restart",			wm_restart),
	FN(global_s,	"exec",				wm_exec),
	FN(global,		"exit",				wm_exit),
	FN(show_menu,	"menu",				NULL),
	FN(global_i,	"goto_frame",		goto_frame),
	FN(global,		"goto_previous",	goto_previous),
	
	/* dock */
	FN(global,		"toggle_dock",		dock_toggle_hide),
	FN(global,		"toggle_dock_dir",	dock_toggle_dir),
					
	/* menu */
	FNI(menu_cmd,	"menu_next",		MENU_CMD_NEXT),
	FNI(menu_cmd,	"menu_prev",		MENU_CMD_PREV),
	FNI(menu_cmd,	"menu_entersub",	MENU_CMD_ENTERSUB),
	FNI(menu_cmd,	"menu_leavesub",	MENU_CMD_LEAVESUB),
	FNI(menu_cmd,	"menu_close",		MENU_CMD_CLOSE),
	FNI(menu_cmd,	"menu_keep",		MENU_CMD_KEEP),
	FNI(menu_cmd,	"menu_raisekeep",	MENU_CMD_RAISEKEEP),
	FNI(menu_cmd,	"menu_execute",		MENU_CMD_EXECUTE),

	/* moveres-mode */
	FN(winobj,		"kb_moveres_end",	keyboard_moveres_end),
	FN(winobj,		"kb_moveres_cancel",keyboard_moveres_cancel),
	FN(winobj_i,	"kb_move",			keyboard_move),
	FN(winobj_i,	"kb_move_stepped",	keyboard_move_stepped),
	FN(frame_i,		"kb_resize",		keyboard_resize),
	FN(frame_i,		"kb_resize_stepped",keyboard_resize_stepped),

	/* mouse move/resize and tab drag */
	FNI(drag,		"move",				DRAG_MOVE),
	FNI(drag,		"resize",			DRAG_RESIZE),
	FNI(drag,		"move_stepped",		DRAG_MOVE_STEPPED),
	FNI(drag,		"resize_stepped",	DRAG_RESIZE_STEPPED),
	FNI(drag,		"tab_drag", 		DRAG_TAB),
	FN(cwin,		"tab_switch",		wrap_switch_clientwin),

#ifdef CF_PACK_MOVE
	FN(winobj_i,    "pack_move",        pack_move),
	FN(winobj_i,    "gotodir",         	gotodir),
#endif

#if 0
	FN(frame,		"hide",				hide_frame),
	FN(global_i,	"sel_dir_frame",	activate_frame_dir),
#endif
	{NULL, NULL, NULL, 0}
};


#undef FNI
#undef FN


/* */


WFunction *lookup_func(const char *name, int arg_type)
{
	WFunction *func=funcs;
	
	while(func->fclass!=NULL){
		if(strcmp(func->fname, name)!=0){
			func++;
			continue;
		}
		if(func->fclass->arg_type!=arg_type)
			return NULL;
		
		return func;
	}
	
	return NULL;
}


/* */


/* winobj-general */


static void winobj_gen_handler(WThing *thing, WFunction *func,
							   WFuncArg arg, int type)
{
	typedef void Func(WWinObj *winobj);
	
	WWinObj *winobj;
	Func *fn;
	
	if(thing==NULL)
		return;
	   
	if((winobj=winobj_of(thing))==NULL)
		return;
	
	if(!WTHING_IS(winobj, type))
		return;
	
	fn=(Func*)(func->func);
	
	fn(winobj);
}


static void winobj_gen_i_handler(WThing *thing, WFunction *func, WFuncArg arg,
								 int type)
{
	typedef void Func(WWinObj *winobj, int arg);
	
	WWinObj *winobj;
	Func *fn;
	
	if(thing==NULL)
		return;

	if((winobj=winobj_of(thing))==NULL)
		return;
	
	if(!WTHING_IS(winobj, type))
		return;
	
	fn=(Func*)(func->func);
	
	fn(winobj, ARG_TO_INT(arg));
}


/* winobj */


static void winobj_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	winobj_gen_handler(thing, func, arg, WTHING_WINOBJ);
}


static void winobj_i_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	winobj_gen_i_handler(thing, func, arg, WTHING_WINOBJ);
}


/* frame */


static void frame_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	winobj_gen_handler(thing, func, arg, WTHING_FRAME);
}


static void frame_i_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	winobj_gen_i_handler(thing, func, arg, WTHING_FRAME);
}


/* global */


static void global_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	typedef void Func();
	
	Func *fn;
	
	fn=(Func*)(func->func);
	
	fn();
}


static void global_i_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	typedef void Func(int arg);
	
	Func *fn;
	
	fn=(Func*)(func->func);

	fn(ARG_TO_INT(arg));
}


static void global_s_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	typedef void Func(const char*p);
	
	Func *fn;
	
	fn=(Func*)(func->func);
	
	fn(ARG_TO_STRING(arg));
}


/* menu */


static void show_menu_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	WMenuData *mdata;
	int x, y;
	
	mdata=lookup_menudata(ARG_TO_STRING(arg));
	
	if(mdata==NULL)
		return;
	
	get_pointer_rootpos(&x, &y);
	
	show_menu(mdata, thing, x, y, FALSE);
}


void show_menu_but_handler(WThing *thing, XButtonEvent *ev,
						   WFunction *func, WFuncArg arg)
{
	WMenuData *mdata;
	
	mdata=lookup_menudata(ARG_TO_STRING(arg));
	
	if(mdata==NULL){
		warn("Menu %s not found", ARG_TO_STRING(arg));
		return;
	}
	
	show_menu(mdata, thing, ev->x_root, ev->y_root, ev->type==ButtonPress);
}


static void menu_i_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	if(thing==NULL)
		return;
	
	if(!WTHING_IS(thing, WTHING_MENU))
		return;
	
	menu_command((WMenu*)thing, func->opval);
}


/* clientwin */


static void cwin_handler(WThing *thing, WFunction *func, WFuncArg arg)
{
	typedef void Func(WClientWin *cwin);
	
	WClientWin *cwin;
	Func *fn;
	
	if(thing==NULL)
		return;
	   
	if(!WTHING_IS(thing, WTHING_CLIENTWIN)){
		if(WTHING_IS(thing, WTHING_FRAME)){
			thing=(WThing*)(((WFrame*)thing)->current_cwin);
			if(thing==NULL)
				return;
		}else{
			return;
		}
	}
	
	cwin=(WClientWin*)thing;
	fn=(Func*)(func->func);
	
	fn(cwin);
}


/*
 * Wrappers
 */


static void wrap_switch_clientwin(WClientWin *cwin)
{
	frame_switch_clientwin(CWIN_FRAME(cwin), cwin);
        if (!WFRAME_IS_SHADE(CWIN_FRAME(cwin)))
		set_frame_state(CWIN_FRAME(cwin), 0);
}


static void wrap_set_stack_lvl(WFrame *frame, int lvl)
{
	if(lvl<0 || lvl>=LVL_MENU)
		return;
	
	restack_winobj((WWinObj*)frame, lvl, TRUE);
}
