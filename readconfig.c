/*
 * pwm/readconfig.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <unistd.h>

#include <libtu/parser.h>
#include <libtu/tokenizer.h>

#include "readconfig.h"
#include "function.h"
#include "binding.h"
#include "winprops.h"
#include "frameid.h"


static uint default_mod=0;
static WScreen *tmp_screen;
static WMenuData *tmp_menudata=NULL;
static WWinProp *tmp_winprop=NULL;
static int tmp_actx=0;
static uint tmp_state=0;
static int tmp_button=0;
static WFuncBinder tmp_pressb={NULL, INIT_NULL_ARG, ARGTYPE_NONE},
				   tmp_clickb={NULL, INIT_NULL_ARG, ARGTYPE_NONE},
                   tmp_dblclickb={NULL, INIT_NULL_ARG, ARGTYPE_NONE},
				   tmp_motionb={NULL,  INIT_NULL_ARG, ARGTYPE_NONE};

#define BUTTON1_NDX 8

static const char *state_names[]={
	"Shift", "Lock", "Control",
	"Mod1", "Mod2", "Mod3", "Mod4", "Mod5",
	"Button1", "Button2", "Button3", "Button4", "Button5", "AnyButton",
	NULL
};

static int state_values[]={
	ShiftMask, LockMask, ControlMask,
	Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask,
	Button1, Button2, Button3, Button4, Button5, AnyButton
};


/*
 * Helpers
 */


static bool get_arg(Tokenizer *tokz, Token *tok, WFuncBinder *binder_ret)
{
	if(TOK_IS_STRING(tok) || TOK_IS_IDENT(tok)){
		SET_STRING_ARG(binder_ret->arg, TOK_TAKE_STRING_VAL(tok));
		binder_ret->arg_type=ARGTYPE_STRING;
		return TRUE;
	}
	
	if(TOK_IS_LONG(tok)){
		SET_INT_ARG(binder_ret->arg, TOK_LONG_VAL(tok));
		binder_ret->arg_type=ARGTYPE_INT;
		return TRUE;
	}

	warn_obj_line(tokz->name, tok->line, "Invalid argument type");
	return FALSE;
}


static bool get_func(Tokenizer *tokz, Token *toks, int n,
					 WFuncBinder *binder_ret)
{
	const char *name=TOK_STRING_VAL(&(toks[0]));
	
	if(n>=2){
		if(!get_arg(tokz, &(toks[1]), binder_ret))
			return FALSE;
	}
	
	binder_ret->func=lookup_func(name, binder_ret->arg_type);
	
	if(binder_ret->func==NULL){
		warn_obj_line(tokz->name, toks[0].line,
					  "Unable to lookup function \"%s\". "
					  "Function unknown or invalid argument type.", name);
		return FALSE;
	}

	return TRUE;
}


static void free_binder(WFuncBinder *binder, bool argstring)
{
	if(argstring &&
	   binder->arg_type==ARGTYPE_STRING && ARG_TO_STRING(binder->arg)!=NULL)
		free(ARG_TO_STRING(binder->arg));
	
	binder->func=NULL;
	binder->arg_type=ARGTYPE_NONE;
	SET_NULL_ARG(binder->arg);
}


static void reset_tmp(bool argstring)
{
	WMenuData *mdata;
	
	tmp_state=0;
	tmp_button=0;
	tmp_actx=0;

	free_binder(&tmp_pressb, argstring);
	free_binder(&tmp_clickb, argstring);
	free_binder(&tmp_dblclickb, argstring);
	free_binder(&tmp_motionb, argstring);
	
	mdata=tmp_menudata;
	
	if(mdata!=NULL){
		tmp_menudata=mdata->menudata_prev;
		free_user_menudata(mdata);
	}
}


static int find_ndx(const char *names[], int num, const char *p)
{
	int i;
	for(i=0; (num<0 || i<num) && names[i]!=NULL; i++){
		if(strcmp(p, names[i])==0)
			return i;
	}
	return -1;
}


/*
 * Keybindings
 */


static bool parse_key(Tokenizer *tokz, Token *tok,
					  uint *mod_ret, int *keysym_ret)
{
	
	char *p=TOK_STRING_VAL(tok);
	char *p2;
	int keysym, i;
	
	while(*p!='\0'){
		p2=strchr(p, '+');
		
		if(p2!=NULL)
			*p2='\0';
		
		keysym=XStringToKeysym(p);
		
		if(keysym!=NoSymbol){
			if(*keysym_ret!=NoSymbol){
				warn_obj_line(tokz->name, tok->line,
							  "Insane key combination");
				return FALSE;
			}
			*keysym_ret=keysym;
		}else{
			i=find_ndx(state_names, BUTTON1_NDX+1, p);
			
			if(i<0){
				warn_obj_line(tokz->name, tok->line,
							  "Unknown key \"%s\"", p);
				return FALSE;
			}
			*mod_ret|=state_values[i];
		}
		
		if(p2==NULL)
			break;
		
		p=p2+1;
	}
	
	return TRUE;
}


/* bind "key", "func" [, args] */
static bool do_kbind(Tokenizer *tokz, int n, Token *toks, uint actx)
{
	uint mod=default_mod;
	int keysym=NoSymbol;
	WFuncBinder binder={NULL, NULL, ARGTYPE_NONE};
	
	if(!parse_key(tokz, &(toks[1]), &mod, &keysym))
		return TRUE;
	
	if(!get_func(tokz, &(toks[2]), n-2, &binder))
		return TRUE; /* just ignore the error */ 
	
	if(add_binding(ACT_KEYPRESS, actx, mod, keysym, &binder))
		return TRUE;
	
	warn_obj_line(tokz->name, toks[0].line, "Unable to bind \"%s\" to \"%s\"", 
			 TOK_STRING_VAL(&(toks[1])), TOK_STRING_VAL(&(toks[2])));
	
	return TRUE;
}


static bool opt_set_mod(Tokenizer *tokz, int n, Token *toks)
{
	uint mod=0;
	int keysym=NoSymbol;
	
	if(!parse_key(tokz, &(toks[1]), &mod, &keysym))
		return FALSE; 

	default_mod=mod;
	
	return TRUE;
}


static bool opt_kbind(Tokenizer *tokz, int n, Token *toks)
{
	return do_kbind(tokz, n, toks, ACTX_GLOBAL);
}


static bool opt_kbind_menu(Tokenizer *tokz, int n, Token *toks)
{
	return do_kbind(tokz, n, toks, ACTX_MENU);
}


static bool opt_kbind_moveres(Tokenizer *tokz, int n, Token *toks)
{
	return do_kbind(tokz, n, toks, ACTX_MOVERES);
}


/*
 * Pointer bindings
 */


static bool opt_mbind_context(Tokenizer *tokz, int n, Token *toks)
{
	static char *context_names[]={
		"root", "tab", "corner", "side", "window",
		"dockwin", "frame", "menu", "menu_title", NULL
	};
	
	static uint context_masks[]={
		ACTX_ROOT, ACTX_TAB, ACTX_CORNER, ACTX_SIDE, ACTX_WINDOW,
		ACTX_DOCKWIN, ACTX_C_FRAME, ACTX_MENU, ACTX_MENUTITLE
	};
	
	int i, j;
	const char *p;

	for(i=1; i<n; i++){
		if(!TOK_IS_IDENT(&(toks[i])))
		   	goto err;
		p=TOK_IDENT_VAL(&(toks[i]));
		for(j=0; context_names[j]!=NULL; j++){
			if(strcmp(context_names[j], p)==0)
				break;
		}
		if(context_names[j]==NULL)
			goto err;
		tmp_actx|=context_masks[j];
	}
	
	return TRUE;
	
err:
	warn_obj_line(tokz->name, toks[i].line, "Invalid context");
	return FALSE;
}


static bool opt_mbind_state(Tokenizer *tokz, int n, Token *toks)
{
	
	int i, j;
	const char *p;

	for(i=1; i<n; i++){
		if(!TOK_IS_IDENT(&(toks[i])))
		   	goto err;
		p=TOK_IDENT_VAL(&(toks[i]));
		j=find_ndx(state_names, -1, p);
		if(j<0)
			goto err;
		if(j>=BUTTON1_NDX)
			tmp_button=state_values[j];
		else
			tmp_state|=state_values[j];
	}
	
	return TRUE;
	
err:
	warn_obj_line(tokz->name, toks[i].line, "Invalid state");
	return TRUE;
}


#define do_mbind_handler(tokz, n, toks, binder) \
			get_func(tokz, &(toks[1]), n-1, binder);


static bool opt_mbind_press(Tokenizer *tokz, int n, Token *toks)
{
	return do_mbind_handler(tokz, n, toks, &tmp_pressb);
}


static bool opt_mbind_click(Tokenizer *tokz, int n, Token *toks)
{
	return do_mbind_handler(tokz, n, toks, &tmp_clickb);
}


static bool opt_mbind_dblclick(Tokenizer *tokz, int n, Token *toks)
{
	return do_mbind_handler(tokz, n, toks, &tmp_dblclickb);
}


static bool opt_mbind_motion(Tokenizer *tokz, int n, Token *toks)
{
	return do_mbind_handler(tokz, n, toks, &tmp_motionb);
}


static bool cancel_mbind(Tokenizer *tokz, int n, Token *toks)
{	
	reset_tmp(FALSE);
	return TRUE;
}


static bool end_mbind(Tokenizer *tokz, int n, Token *toks)
{
	bool retval;
	
	retval=add_pointer_binding(tmp_actx, tmp_state, tmp_button,
							   &tmp_pressb, &tmp_clickb,
							   &tmp_dblclickb, &tmp_motionb);
	
	if(retval==FALSE)
		warn_obj_line(tokz->name, tokz->line, "Unable to bind pointer");
	
	reset_tmp(FALSE);
	return TRUE;
}


/*
 * Menus
 */


static bool alloc_entry(WMenuData *mdata)
{
	WMenuEnt *tmp;
	
	if(mdata->entries==NULL){
		mdata->entries=ALLOC(WMenuEnt);
		if(mdata->entries==NULL)
			return FALSE;
	}else{
		tmp=realloc(mdata->entries, sizeof(WMenuEnt)*(mdata->nentries+1));
		if(tmp==NULL)
			return FALSE;
		
		mdata->entries=tmp;
		tmp+=mdata->nentries;
		
		/* The rest of the settings are set appropriately by the caller */
		tmp->flags=0;
	}
	
	mdata->nentries++;
	
	return TRUE;
}


static bool opt_menu_entry(Tokenizer *tokz, int n, Token *toks)
{
	WFuncBinder binder={NULL, NULL, ARGTYPE_NONE};
	char *entryname=TOK_TAKE_STRING_VAL(&(toks[1]));
	WMenuEnt *entry;
	
	if(!get_func(tokz, &(toks[2]), n-2, &binder))
		goto err;
	
	if(!alloc_entry(tmp_menudata)){
		warn_err();
		goto err;
	}
	
	entry=&(tmp_menudata->entries[tmp_menudata->nentries-1]);	
	
	entry->name=entryname;
	entry->u.f.function=binder.func;
	entry->u.f.arg=binder.arg;
	entry->u.f.arg_type=binder.arg_type;
	
	return TRUE;

err:
	if(entryname!=NULL)
		free(entryname);
	free_binder(&binder, TRUE);
	
	return FALSE;
}


static bool opt_menu_contextual(Tokenizer *tokz, int n, Token *toks)
{
	tmp_menudata->flags|=WMENUDATA_CONTEXTUAL;
	
	return TRUE;
}


static bool add_submenuent(WMenuData *mdata, WMenuData *submenu)
{
	char *title;
	WMenuEnt *entry;
	
	title=scopy(submenu->title);
	
	if(title==NULL){
		warn_err();
		return FALSE;
	}
	
	if(!alloc_entry(mdata)){
		warn_err();
		free(title);
		return FALSE;
	}
	
	entry=&(mdata->entries[mdata->nentries-1]);
	
	entry->name=title;
	entry->u.submenu=submenu;
	entry->flags|=WMENUENT_SUBMENU;
	
	return TRUE;
}


static bool opt_menu_submenu(Tokenizer *tokz, int n, Token *toks)
{
	char *name=TOK_TAKE_STRING_VAL(&(toks[1]));
	WMenuData *submenu;

	submenu=lookup_menudata(name);
	
	if(submenu==NULL){
		warn_obj_line(tokz->name, toks[1].line, "Unknown menu name");
		goto err;
	}
	
	if(add_submenuent(tmp_menudata, submenu))
		return TRUE;
	
err:	
	if(name!=NULL)
		free(name);
	
	return FALSE;
}


static WMenuData *do_begin_menu(Tokenizer *tokz, int n, Token *toks)
{	
	WMenuData *mdata=ALLOC(WMenuData);
	
	if(mdata==NULL)
		return NULL;
	
	mdata->flags=WMENUDATA_USER;
	mdata->name=TOK_TAKE_STRING_VAL(&(toks[1]));
	
	if(n>2)
		mdata->title=TOK_TAKE_STRING_VAL(&(toks[2]));
	else
		mdata->title=mdata->name;
	
	return mdata;
}


static bool begin_menu(Tokenizer *tokz, int n, Token *toks)
{	
	WMenuData *mdata=do_begin_menu(tokz, n, toks);
	
	if(mdata==NULL)
		return FALSE;
	
	mdata->menudata_prev=tmp_menudata;
	tmp_menudata=mdata;
	
	return TRUE;
}


static bool opt_menu_menu(Tokenizer *tokz, int n, Token *toks)
{
	WMenuData *mdata=do_begin_menu(tokz, n, toks);
	
	if(mdata==NULL)
		return FALSE;
	
	if(!add_submenuent(tmp_menudata, mdata)){
		free_user_menudata(mdata);
		return FALSE;
	}

	mdata->menudata_prev=tmp_menudata;
	tmp_menudata=mdata;
	
	return TRUE;
}


static bool cancel_menu(Tokenizer *tokz, int n, Token *toks)
{	
	reset_tmp(TRUE);
	return TRUE;
}


static bool end_menu(Tokenizer *tokz, int n, Token *toks)
{
	WMenuData *mdata;
	
	assert(tmp_menudata!=NULL);
	
	mdata=tmp_menudata->menudata_prev;
	
	if(register_menudata(tmp_menudata)){
		tmp_menudata=mdata;
		return TRUE;
	}
	
	warn_obj_line(tokz->name, tokz->line, "Unable to create menu");
	reset_tmp(TRUE);
	return FALSE;
}


/*
 * Screen (graphics)
 */


static bool opt_screen_font(Tokenizer *tokz, int n, Token *toks)
{
	if(tmp_screen==NULL)
		return TRUE;

	GRDATA->font=load_font(wglobal.dpy, TOK_STRING_VAL(&(toks[1])));
	
	return TRUE;
}


static bool opt_screen_menu_font(Tokenizer *tokz, int n, Token *toks)
{
	if(tmp_screen==NULL)
		return TRUE;
	
	GRDATA->menu_font=load_font(wglobal.dpy, TOK_STRING_VAL(&(toks[1])));
	
	return TRUE;
}
	

static int opt_screen_border_w(Tokenizer *tokz, int n, Token *toks)
{
	int i, j;
	
	if(tmp_screen==NULL)
		return TRUE;
	
	i=TOK_LONG_VAL(&(toks[1]));
	j=TOK_LONG_VAL(&(toks[2]));
	
	if(i<0 || j<0 || j*2>i){
		warn_obj_line(tokz->name, toks[1].line, "Erroneous values");
		return FALSE;
	}
	
	GRDATA->border_width=i;
	GRDATA->bevel_width=j;
	
	return TRUE;
}


static int opt_screen_bar_w(Tokenizer *tokz, int n, Token *toks)
{
	int i, k;
	float j;
	
	if(tmp_screen==NULL)
		return TRUE;
	
	i=TOK_LONG_VAL(&(toks[1]));
	j=TOK_DOUBLE_VAL(&(toks[2]));
	k=TOK_LONG_VAL(&(toks[3]));
	
	if(i<0 || j<0.0 || k<0 || k>i){
		warn_obj_line(tokz->name, toks[1].line, "Erroneous values");
		return FALSE;
	}
	
	GRDATA->bar_min_width=i;
	GRDATA->bar_max_width_q=j;
	GRDATA->tab_min_width=k;
	
	return TRUE;
}
	

static bool do_colorgroup(Tokenizer *tokz, Token *toks, WColorGroup *cg)
{
	int cnt=0;
	
	if(tmp_screen==NULL)
		return TRUE;
	
	cnt+=alloc_color(TOK_STRING_VAL(&(toks[1])), &(cg->pixels[WCG_PIX_HL]));
	cnt+=alloc_color(TOK_STRING_VAL(&(toks[2])), &(cg->pixels[WCG_PIX_SH]));
	cnt+=alloc_color(TOK_STRING_VAL(&(toks[3])), &(cg->pixels[WCG_PIX_BG]));
	cnt+=alloc_color(TOK_STRING_VAL(&(toks[4])), &(cg->pixels[WCG_PIX_FG]));
	
	if(cnt!=4){
		warn_obj_line(tokz->name, toks[1].line,
					  "Unable to allocate one or more colors");
		return FALSE;
	}
	
	return TRUE;
}

#define CGHAND(CG)                                                \
 static bool opt_screen_##CG(Tokenizer *tokz, int n, Token *toks) \
 {                                                                \
	return do_colorgroup(tokz, toks, &(GRDATA->CG));              \
 }

CGHAND(act_tab_colors)
CGHAND(act_tab_sel_colors)
CGHAND(act_base_colors)
CGHAND(act_sel_colors)
CGHAND(tab_colors)
CGHAND(tab_sel_colors)
CGHAND(base_colors)
CGHAND(sel_colors)

#undef CGHAND


static bool opt_screen_workspaces(Tokenizer *tokz, int n, Token *toks)
{
	int hws, vws=1;
	
	if(tmp_screen==NULL)
		return TRUE;

	hws=TOK_LONG_VAL(&(toks[1]));
	if(n>=3)
		vws=TOK_LONG_VAL(&(toks[2]));
	
	if(hws<=0){
		warn_obj_line(tokz->name, toks[1].line, "Invalid value");
		return FALSE;
	}

	if(vws<=0){
		warn_obj_line(tokz->name, toks[2].line, "Invalid value");
		return FALSE;
	}
	
	tmp_screen->n_workspaces=hws*vws;
	tmp_screen->workspaces_horiz=hws;
	tmp_screen->workspaces_vert=vws;
	
	return TRUE;
}	


static bool opt_screen_opaque_move(Tokenizer *tokz, int n, Token *toks)
{
	int om=TOK_LONG_VAL(&(toks[1]));

	if(tmp_screen==NULL)
		return TRUE;
	
	if(om<0)
		om=0;
	
	tmp_screen->opaque_move=om;
	
	return TRUE;
}


static bool opt_screen_dock(Tokenizer *tokz, int n, Token *toks)
{
	const char *geom=TOK_STRING_VAL(&(toks[1]));
	static char *dock_options[]={ "hidden", "sliding", "horizontal", NULL };
	static uint dock_masks[]={ WWINOBJ_HIDDEN, DOCK_SLIDING, DOCK_HORIZONTAL };
	uint flags=0;
	const char *p;
	int i, j;
	
	if(tmp_screen==NULL)
		return TRUE;

	for(i=2; i<n; i++){
		if(!TOK_IS_IDENT(&(toks[i])))
		   	goto err;
		p=TOK_IDENT_VAL(&(toks[i]));
		for(j=0; dock_options[j]!=NULL; j++){
			if(strcmp(dock_options[j], p)==0)
				break;
		}
		if(dock_options[j]==NULL)
			goto err;
		flags|=dock_masks[j];
	}
	set_dock_params(geom, flags);
	return TRUE;

err:
	warn_obj_line(tokz->name, toks[i].line, "Invalid option");
	return FALSE;
}


static bool opt_screen_autoraise_time(Tokenizer *tokz, int n, Token *toks)
{
	int i=TOK_LONG_VAL(&(toks[1]));

	if(tmp_screen==NULL)
		return TRUE;

	if(i<0)
		i=-1;

	GRDATA->autoraise_time=i;

	return TRUE;
}


static bool begin_screen(Tokenizer *tokz, int n, Token *toks)
{	
	int i;
	
	for(i=1; i<n; i++){
		if(TOK_LONG_VAL(&(toks[i]))==SCREEN->xscr){
			tmp_screen=SCREEN;
			break;
		}
	}

	return TRUE;
}


static bool end_screen(Tokenizer *tokz, int n, Token *toks)
{	
	tmp_screen=NULL;
	return TRUE;
}


/* 
 * global
 */


static bool opt_dblclick_delay(Tokenizer *tokz, int n, Token *toks)
{
	int dd=TOK_LONG_VAL(&(toks[1]));

	wglobal.dblclick_delay=(dd<0 ? 0 : dd);
	
	return TRUE;
}


/*
 * Window props
 */


static bool opt_winprop_frame(Tokenizer *tokz, int n, Token *toks)
{
	int f=TOK_LONG_VAL(&(toks[1]));
	
	if(f>FRAME_ID_START_CLIENT){
		warn_obj_line(tokz->name, toks[1].line, "Frame ID should be < %d\n",
					  FRAME_ID_START_CLIENT);
		return FALSE;
	}
	
	tmp_winprop->init_frame=f;
	
	return TRUE;
}


static bool opt_winprop_workspace(Tokenizer *tokz, int n, Token *toks)
{
	int w=TOK_LONG_VAL(&(toks[1]));
	
	if(w<WORKSPACE_STICKY){
		warn_obj_line(tokz->name, toks[1].line, "Invalid workspace number");
		return FALSE;
	}
	
	tmp_winprop->init_workspace=w;
	
	return TRUE;
}


static bool opt_winprop_dockpos(Tokenizer *tokz, int n, Token *toks)
{
	int p=TOK_LONG_VAL(&(toks[1]));
	
	tmp_winprop->dockpos=p;
	
	return TRUE;
}


static bool opt_winprop_wildmode(Tokenizer *tokz, int n, Token *toks)
{
	char *s=TOK_STRING_VAL(&(toks[1]));
	
	if(strcmp(s, "app")==0){
		tmp_winprop->wildmode=WILDMODE_APP;
	}else if(strcmp(s, "yes")==0){
		tmp_winprop->wildmode=WILDMODE_YES;
	}else if(strcmp(s, "no")==0){
		tmp_winprop->wildmode=WILDMODE_NO;
	}else{
		warn_obj_line(tokz->name, toks[1].line, "Invalid mode");
		return FALSE;
	}
	
	return TRUE;
}


static bool begin_winprop(Tokenizer *tokz, int n, Token *toks)
{
	WWinProp *wrop;
	char *wclass, *winstance;
	
	tmp_winprop=ALLOC(WWinProp);
	
	if(tmp_winprop==NULL){
		warn_err();
		return FALSE;
	}
	
	tmp_winprop->init_workspace=WORKSPACE_CURRENT;
	tmp_winprop->init_frame=0;
	
	tmp_winprop->data=wclass=TOK_TAKE_STRING_VAL(&(toks[1]));
	
	winstance=strchr(wclass, '.');

	if(winstance!=NULL){
		*winstance++='\0';
		if(strcmp(winstance, "*")==0)
			winstance=NULL;
	}
	
	if(strcmp(wclass, "*")==0)
		wclass=NULL;
	
	tmp_winprop->wclass=wclass;
	tmp_winprop->winstance=winstance;
	
	return TRUE;
}
	

static bool end_winprop(Tokenizer *tokz, int n, Token *toks)
{
	register_winprop(tmp_winprop);
	tmp_winprop=NULL;
	
	return TRUE;
}


static bool cancel_winprop(Tokenizer *tokz, int n, Token *toks)
{
	free_winprop(tmp_winprop);
	tmp_winprop=NULL;
	
	return TRUE;
}


		   
/* */


static ConfOpt mbind_opts[]={
	/* when */
	{"context", "i+", opt_mbind_context, NULL},
	{"state", "i+", opt_mbind_state, NULL},
	
	/* actions */
	{"press", "s?.", opt_mbind_press, NULL},
	{"click", "s?.", opt_mbind_click, NULL},
	{"dblclick", "s?.", opt_mbind_dblclick, NULL},
	{"motion", "s?.", opt_mbind_motion, NULL},
	
	{"#end", NULL, end_mbind, NULL},
	{"#cancel", NULL, cancel_mbind, NULL},
	
	{NULL, NULL, NULL, NULL}
};


static ConfOpt menu_opts[]={
	{"entry", "ss?.", opt_menu_entry, NULL},
	{"menu", "s?s", opt_menu_menu, menu_opts},
	{"submenu", "s", opt_menu_submenu, NULL},
	{"contextual", NULL, opt_menu_contextual, NULL},

	{"#end", NULL, end_menu, NULL},
	{"#cancel", NULL, cancel_menu, NULL},

	{NULL, NULL, NULL, NULL}
};


static ConfOpt screen_opts[]={
	{"menu_font", "s", opt_screen_menu_font, NULL},
	{"font", "s", opt_screen_font, NULL},
	{"border_w", "ll", opt_screen_border_w, NULL},
	{"bar_w", "ldl", opt_screen_bar_w, NULL},
	{"act_tab_colors", "ssss", opt_screen_act_tab_colors, NULL},
	{"act_tab_sel_colors", "ssss", opt_screen_act_tab_sel_colors, NULL},
	{"act_base_colors", "ssss", opt_screen_act_base_colors, NULL},
	{"act_sel_colors", "ssss", opt_screen_act_sel_colors, NULL},
	{"tab_colors", "ssss", opt_screen_tab_colors, NULL},
	{"tab_sel_colors", "ssss", opt_screen_tab_sel_colors, NULL},
	{"base_colors", "ssss", opt_screen_base_colors, NULL},
	{"sel_colors", "ssss", opt_screen_sel_colors, NULL},
	{"workspaces", "l?l", opt_screen_workspaces, NULL},
	{"opaque_move", "l", opt_screen_opaque_move, NULL},
	{"dock", "s?i+", opt_screen_dock, NULL},
	{"autoraise_time", "l", opt_screen_autoraise_time, NULL},
	
	{"#end", NULL, end_screen, NULL},
	{"#cancel", NULL, end_screen, NULL},
	
	{NULL, NULL, NULL, NULL}
};


static ConfOpt winprop_opts[]={
	{"frame", "l", opt_winprop_frame, NULL},
	{"workspace", "l", opt_winprop_workspace, NULL},
	{"dockpos", "l", opt_winprop_dockpos, NULL},
	{"wildmode", "i", opt_winprop_wildmode, NULL},
	
	{"#end", NULL, end_winprop, NULL},
	{"#cancel", NULL, cancel_winprop, NULL},
	
	{NULL, NULL, NULL, NULL}
};


static ConfOpt toplevel_opts[]={
	/* keybindings */
	{"kbind", "ss?.", opt_kbind, NULL},
	{"kbind_menu", "ss?.", opt_kbind_menu, NULL},
	{"kbind_moveres", "ss?.", opt_kbind_moveres, NULL},
	{"set_mod", "s", opt_set_mod, NULL},

	/* mouse bindings */
	{"mbind", NULL, NULL, mbind_opts},

	/* menus */
	{"menu", "s?s", begin_menu, menu_opts},

	/* screens */
	{"screen", "l+", begin_screen, screen_opts},
	
	{"dblclick_delay", "l", opt_dblclick_delay, NULL},
	
	/* window props */
	{"winprop" , "s", begin_winprop, winprop_opts},
	
	{NULL, NULL, NULL, NULL}
};


/* */


static char *includepaths[]={
	CF_SYS_CONFIG_LOCATION, NULL
};


bool read_config(const char* cfgfile)
{
	char *tmp=NULL;
	bool retval=FALSE;
	Tokenizer *tokz;
	
	default_mod=0;
	
	if(cfgfile==NULL){
		cfgfile=getenv("HOME");
		if(cfgfile!=NULL){
			tmp=ALLOC_N(char, strlen(cfgfile)+strlen(CF_USER_CFGFILE)+2);
			sprintf(tmp, "%s/%s", cfgfile, CF_USER_CFGFILE);
			cfgfile=tmp;
		}
		
		if(cfgfile==NULL || access(cfgfile, F_OK)!=0)
			cfgfile=CF_GLOBAL_CFGFILE;
	}

	tokz=tokz_open(cfgfile);
	    
	if(tokz!=NULL){
		tokz->flags=TOKZ_ERROR_TOLERANT;
		tokz_set_includepaths(tokz, includepaths);
		retval=parse_config_tokz(tokz, toplevel_opts);
		tokz_close(tokz);
	}
	
	if(tmp!=NULL)
		free(tmp);
	
	return retval;
}
