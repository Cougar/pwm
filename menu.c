/*
 * pwm/menu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "menu.h"
#include "font.h"
#include "winobj.h"
#include "draw.h"
#include "pointer.h"
#include "event.h"
#include "focus.h"
#include "moveres.h"
#include "binding.h"
#include "signal.h"


static int textwidth(XFontStruct *font, const char *str)
{
	return XTextWidth(font, str, strlen(str));
}

static void menu_set_selected(WMenu *menu, int selected);

/* */

static bool test_scroll(WMenu *menu, int rx, int ry);
static void start_scroll(WMenu *menu);
static void end_scroll();
static bool is_vis_entry(WMenu *menu, int entry, int *dxret, int *dyret);
static void vis_entry(WMenu *menu, int entry);

/* */


/* Calculate the desired width of a menu window
 */
static int menu_width(WMenuData *mdata, int flags)
{
	int i=0, maxw=0, inc=0, w=0;
	WMenuEnt *ent=mdata->entries;
	
	if(!(flags&WMENU_NOTITLE))
		maxw=textwidth(GRDATA->font, mdata->title)+2*CF_MENUTITLE_H_SPACE
			 +GRDATA->stick_pixmap_w;
		
	for(; i<mdata->nentries; i++, ent++){
		w=textwidth(GRDATA->menu_font, ent->name);
		w+=2*CF_MENUENT_H_SPACE;
		if(ent->flags&WMENUENT_SUBMENU)
			inc=GRDATA->submenu_ind_w+CF_SUBMENU_IND_H_SPACE;
		if(w>maxw)
			maxw=w;
	}
	
	return maxw+inc;
}


/* Calculate the desired height of a menu window
 */
static int menu_height(WMenuData *mdata)
{
	int height;

	height=FONT_HEIGHT(GRDATA->menu_font)+2*CF_MENUENT_V_SPACE;
	height*=mdata->nentries;
	height+=CF_MENU_V_SPACE*2;

	return height;
}


enum{
	NO_ENTRY=-1,
	ENTRY_TITLE=-2
};
	
/* Find the entry at (x, y) in menu-window coordinates.
 * A negative value means there is no entry at that coordinate.
 */
static int entry_at(WMenu *menu, int x, int y)
{
	int ent;
	
	if(x<0 || x>menu->w)
		return NO_ENTRY;
	
	if(y<0 || y>menu->h)
		return NO_ENTRY;
	
	y-=menu->title_height+CF_MENU_V_SPACE;

	if(y<0)
		return ENTRY_TITLE;
	
	ent=y/menu->entry_height;
	
	if(ent>=menu->data->nentries)
		ent=NO_ENTRY;
	
	return ent;
}


/* Calculate submenu placement position
 */
static void submenu_pos(WMenu *menu, int entry, int *xret, int *yret)
{
	int x, y;
	WMenuData *submdata;
	
	submdata=SUBMENU_DATA(&(menu->data->entries[entry]));
	
	x=menu->x+menu->w;
	y=menu->y+menu->title_height+CF_MENU_V_SPACE
	  +entry*menu->entry_height;
	
	*xret=x;
	*yret=y;
}


/* Create a menu window
 */
static WMenu* create_menu(WMenuData *mdata, WThing *context, int x, int y,
						  WMenu *parent)
{
	int w, h, th, eh;
	int flags=0;
	Window win;
	WMenu *menu;
	
	if(mdata->init_func!=NULL && mdata->nref==0)
		mdata->init_func(mdata, context);

	if(mdata->flags&WMENUDATA_CONTEXTUAL){
		if(context==NULL || WTHING_IS(context, WTHING_SCREEN) ||
		   WTHING_IS(context, WTHING_MENU))
			return NULL;
		flags|=(WMENU_NOTITLE|WMENU_CONTEXTUAL);
	}else{
		context=NULL;
	}
	
	if(mdata->nref!=0)
		flags|=(WMENU_NOTITLE|WMENU_CONTEXTUAL);
	
	if(mdata->title==NULL)
		flags|=(WMENU_NOTITLE|WMENU_CONTEXTUAL);
	
	if(parent!=NULL)
		flags|=parent->flags&(WMENU_NOTITLE|WMENU_CONTEXTUAL);

	/* */
	
	w=menu_width(mdata, flags);
	h=menu_height(mdata);	
	
	/* Don't display empty menus */
	if(w==0 || h==0)
		return NULL;
	
	/* */
	
	menu=ALLOC(WMenu);
	
	if(menu==NULL){
		warn_err();
		return NULL;
	}
	
	WTHING_INIT(menu, WTHING_MENU);
	
	th=FONT_HEIGHT(GRDATA->font)+2*CF_MENUTITLE_V_SPACE;
	eh=FONT_HEIGHT(GRDATA->menu_font)+2*CF_MENUENT_V_SPACE;

	if(parent==NULL){
		x-=w/2;
		y+=th/(flags&WMENU_NOTITLE ? 2 : -2 );
	}else if(!(flags&WMENU_NOTITLE)){
		y-=th;
	}

	if(flags&WMENU_NOTITLE)
		th=0;

	h+=th;

	win=create_simple_window(x, y, w, h,
							 GRDATA->base_colors.pixels[WCG_PIX_BG]);
	
	menu->menu_win=win;
	menu->x=x;
	menu->y=y;
	menu->w=w;
	menu->h=h;
	menu->title_height=th;
	menu->entry_height=eh;
	menu->flags=flags;
	menu->selected=NO_ENTRY;
	menu->data=mdata;
	menu->context=context;
	
	if(mdata->nref++==0)
		mdata->inst1=menu;

	XSelectInput(wglobal.dpy, win, MENU_MASK);
	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)menu);

	if(parent==NULL)
		add_winobj((WWinObj*)menu, WORKSPACE_STICKY, LVL_MENU);
	else
		add_winobj_above((WWinObj*)menu, (WWinObj*)parent);

	map_winobj((WWinObj*)menu);

	return menu;
}


static void update_menu(WMenu *menu)
{
	int w, h;
	
	w=menu_width(menu->data, menu->flags);
	h=menu_height(menu->data);	
	h+=menu->title_height;
	
	if(menu->w!=w || menu->h!=h){
		menu->w=w;
		menu->h=h;
		XResizeWindow(wglobal.dpy, menu->menu_win, w, h);
	}else{
		draw_menu(menu, TRUE);
	}
}


void update_menus(WMenuData *mdata)
{
	WMenu *menu;
	
	menu=(WMenu*)subthing((WThing*)SCREEN, WTHING_MENU);
	
	while(menu!=NULL){
		if(menu->data==mdata)
			update_menu(menu);
		menu=(WMenu*)next_thing((WThing*)menu, WTHING_MENU);
	}
}


/* */


static void do_destroy_menu(WMenu *menu)
{
	if(--menu->data->nref==0 && menu->data->deinit_func!=NULL)
		menu->data->deinit_func(menu->data);
	
	if(menu->data->inst1==menu)
		menu->data->inst1=NULL;
	
	unlink_winobj_d((WWinObj*)menu);
	XDeleteContext(wglobal.dpy, menu->menu_win, wglobal.win_context);
	XDestroyWindow(wglobal.dpy, menu->menu_win);
	free_thing((WThing*)menu);
}
	

/* Destroy a menu and all its submenus.
 */
static void do_destroy_menu_tree(WMenu *menu, bool setsel,
								 bool contextual_only)
{
	WWinObj *tmpobj=NULL, *p, *next;
	WWinObj *parent=menu->stack_above;
	
	p=init_traverse_winobjs_b((WWinObj*)menu, &tmpobj);
	
	for(; p!=NULL; p=next){
		next=traverse_winobjs_b(p, (WWinObj*)menu, &tmpobj);
		
		if(!WTHING_IS(p, WTHING_MENU))
			continue;
		
		if(contextual_only && !(p->flags&WMENU_CONTEXTUAL))
			continue;
		
		/* destroy the menu */
		parent=p->stack_above;
		do_destroy_menu((WMenu*)p);
		if(setsel && parent!=NULL && WTHING_IS(parent, WTHING_MENU))
			menu_set_selected((WMenu*)parent, NO_ENTRY);
	}
}


void destroy_menu_tree(WMenu *menu)
{
	do_destroy_menu_tree(menu, TRUE, FALSE);
}
		 
		 
/* Destroy a complete menu tree starting from the root.
 */
static void destroy_menu_root(WMenu *menu, bool all)
{
	do{
		if(menu->stack_above==NULL)
			break;
		
		if(!all && menu->stack_above->flags&WMENU_KEEP)
			break;
		
		menu=(WMenu*)menu->stack_above;
	}while(1);
	
	destroy_menu_tree(menu);
}


void destroy_contextual_menus()
{
	WWinObj *next, *obj=SCREEN->winobj_stack_lists[LVL_MENU];
	
	while(obj!=NULL){
		next=obj->stack_next;
		
		if(WTHING_IS(obj, WTHING_MENU))
			do_destroy_menu_tree((WMenu*)obj, TRUE, TRUE);
		
		obj=next;
	}
}


static void finish_contextual_menus()
{
	if(wglobal.input_mode==INPUT_CTXMENU)
		ungrab_kb_ptr();
	
	destroy_contextual_menus();
}


static void finish_menu(WMenu *menu, bool all)
{
	menu->flags&=~WMENU_KEEP;
	destroy_menu_root(menu, all);
}


/* Select given entry
 */
static void menu_set_selected(WMenu *menu, int selected)
{
	WMenuEnt *entry;
	
	if(menu->data->nentries<=selected)
		selected=NO_ENTRY;
	
	if(MENU_SUBMENU(menu)!=NULL)
		do_destroy_menu_tree(MENU_SUBMENU(menu), FALSE, FALSE);

	if(menu->selected==selected)
		return;
	
	erase_menu_selection(menu);	
	menu->selected=selected;
	draw_menu_selection(menu);
	
	return;
}


/* Create submenu window 
 */
static WMenu *do_show_selected_submenu(WMenu *menu, WMenuEnt *entry)
{
	int x, y;
	WMenu *submenu;

	submenu_pos(menu, menu->selected, &x, &y);
	
	submenu=create_menu(SUBMENU_DATA(entry), menu->context, x, y, menu);
	
	return submenu;
}


/* If the selected entry is a submenu, show that menu
 */
static WMenu *show_selected_submenu(WMenu *menu)
{
	WMenuEnt *entry;
	
	if(menu->selected<0 || MENU_SUBMENU(menu)!=NULL)
		return MENU_SUBMENU(menu);
		
	entry=&(menu->data->entries[menu->selected]);
	
	if(!(entry->flags&WMENUENT_SUBMENU))
		return NULL;
	
	return do_show_selected_submenu(menu, entry);
}


/* Execute selected entry; either show submenu or
 * call given function.
 */
static bool menu_execute_selected(WMenu *menu)
{
	WMenuEnt *entry;
	WMenuData *data;
	WThing *context;

	if(menu->selected<0)
		return TRUE;
	
	/* Submenu already visible? -> return */
	if(MENU_SUBMENU(menu)!=NULL)
		return TRUE;
	
	data=menu->data;
	context=menu->context;	
	entry=&(data->entries[menu->selected]);
	
	if(entry->flags&WMENUENT_SUBMENU){
		do_show_selected_submenu(menu, entry);
		return TRUE;
	}

	data->nref++;
	
	if(menu->flags&WMENU_KEEP){
		menu_set_selected(menu, NO_ENTRY);
	}else{
		finish_menu(menu, FALSE);
	}
	
	finish_contextual_menus();

	if(data->exec_func!=NULL)
		data->exec_func(entry, context);
	   
	if(--data->nref==0 && data->deinit_func!=NULL)
		data->deinit_func(data);
	
	return FALSE;
}


/* */


static void keep(WMenu *menu)
{
	WWinObj *tmp;
	
	if(menu->flags&(WMENU_KEEP|WMENU_CONTEXTUAL))
		return;
	
	menu->flags|=WMENU_KEEP;

	tmp=menu->stack_above;
	
	if(tmp!=NULL){
		restack_winobj((WWinObj*)menu, LVL_MENU, TRUE);
		menu_set_selected((WMenu*)tmp, NO_ENTRY);
	}
	draw_menu(menu, FALSE);
}


/* */


static void do_move(WMenu *menu, int dx, int dy)
{
	menu->x+=dx;
	menu->y+=dy;
	XMoveWindow(wglobal.dpy, menu->menu_win, menu->x, menu->y);
}


static void move_menu(WMenu *menu, int dx, int dy)
{
	WWinObj *p;
	
	do_move(menu, dx, dy);
	
	p=menu->stack_above_list;

	while(p!=NULL){
		if(WTHING_IS(p, WTHING_MENU))
			do_move((WMenu*)p, dx, dy);
		p=traverse_winobjs(p, (WWinObj*)menu);
	}
}


void set_menu_pos(WMenu *menu, int x, int y)
{
	move_menu(menu, x-menu->x, y-menu->y);
}


/* */


void activate_menu(WMenu *menu)
{
	draw_menu(menu, TRUE);
}


void deactivate_menu(WMenu *menu)
{
	draw_menu(menu, TRUE);
}


static void set_active_menu(WMenu *menu)
{
	if(wglobal.input_mode==INPUT_CTXMENU)
		wglobal.grab_holder=(WThing*)menu;
	else
		set_focus((WThing*)menu);
}


/* */


void menu_command(WMenu *menu, int cmd)
{
	WMenu *other;
	
	switch(cmd){
	case MENU_CMD_PREV:
		if(menu->selected>0)
			menu_set_selected(menu, menu->selected-1);
		else
			menu_set_selected(menu, menu->data->nentries-1);
		vis_entry(menu, menu->selected);
		break;
		
	case MENU_CMD_NEXT:
		if(menu->selected<menu->data->nentries-1)
			menu_set_selected(menu, menu->selected+1);
		else
			menu_set_selected(menu, 0);
		vis_entry(menu, menu->selected);
		break;
		
	case MENU_CMD_ENTERSUB:
	case MENU_CMD_EXECUTE:
		if(menu->selected<0)
			break;
		if(MENU_SUBMENU(menu)==NULL){
			if(menu->data->entries[menu->selected].flags&WMENUENT_SUBMENU)
				show_selected_submenu(menu);
		}
		other=MENU_SUBMENU(menu);
		if(other!=NULL){
			set_active_menu(other);
			if(other->selected==NO_ENTRY)
				other->selected=0;
			vis_entry(other, other->selected);
			break;
		}
		
		if(cmd!=MENU_CMD_EXECUTE)
			break;
		
		menu_execute_selected(menu);
		break;
		
	case MENU_CMD_LEAVESUB:
		other=MENU_PARENT(menu);
		if(other!=NULL){
			do_destroy_menu_tree(menu, FALSE, FALSE);
			/* set_active_menu is not used because unlink_winobj_d gives the
			 * parent a focus if the destroyed menu is current. This should
			 * not bee true in INPUT_CTXMENU mode.
			 */
			if(wglobal.input_mode==INPUT_CTXMENU)
				wglobal.grab_holder=(WThing*)other;
			vis_entry(other, other->selected);
		}
		break;
		
	case MENU_CMD_CLOSE:
		finish_menu(menu, TRUE);
		finish_contextual_menus();
		break;
		
	case MENU_CMD_RAISEKEEP:
		raise_winobj((WWinObj*)menu);
	case MENU_CMD_KEEP:
		keep(menu);
		break;
	}
}


/* */


void show_menu(WMenuData *mdata, WThing *context, int x, int y,
			   bool button_down)
{
	
	WMenu* menu;
	
	if(mdata->inst1!=NULL){
		menu=mdata->inst1;
		if(menu->flags&WMENU_KEEP)
			goto create_it;
		
		destroy_menu_root(menu, FALSE);
		return;
	}
	
create_it:
	menu=create_menu(mdata, context, x, y, NULL);

	if(menu==NULL)
		return;
	
	if(button_down){
		pointer_change_context((WThing*)menu, ACTX_MENU);
	}else if(menu->flags&WMENU_CONTEXTUAL){
		vis_entry(menu, -1);
		grab_kb_ptr();
		wglobal.input_mode=INPUT_CTXMENU;
		wglobal.grab_holder=(WThing*)menu;
	}
}


/* */


void menu_button(WThing *thing, XButtonEvent *ev,
				 WFunction *func, WFuncArg arg)
{
	WMenu *menu;
	int x, y, entry;
	
	if(!WTHING_IS(thing, WTHING_MENU))
		return;
	
	menu=(WMenu*)thing;
	x=ev->x_root-menu->x;
	y=ev->y_root-menu->y;
	entry=entry_at(menu, x, y);
	
	if(ev->type==ButtonPress){
		/* press */
		if(entry==menu->selected){
			menu_set_selected(menu, NO_ENTRY);
		}else{
			menu_set_selected(menu, entry);
			show_selected_submenu(menu);
		}
		return;
	}
	
	end_scroll();
	
	/* release */
	if(entry>=0){
		if(menu->selected==entry)
			menu_execute_selected(menu);
	}else if(entry!=ENTRY_TITLE && !(menu->flags&WMENU_KEEP)){
		finish_menu(menu, FALSE);
	}
	
	destroy_contextual_menus();
}


bool menu_select_at(WMenu *menu, int rx, int ry)
{
	int x, y;
	int entry;
	
	x=rx-menu->x;
	y=ry-menu->y;
	
	entry=entry_at(menu, x, y);
	
	if(entry==NO_ENTRY){
		if(MENU_SUBMENU(menu)==NULL)
			menu_set_selected(menu, NO_ENTRY);
		
		return FALSE;
	}
	
	if(entry!=menu->selected){
		menu_set_selected(menu, entry);
		show_selected_submenu(menu);
	}

	return TRUE;
}
	

void menu_motion(WThing *thing, XMotionEvent *ev, int dx, int dy,
				 WFunction *func, WFuncArg arg)
{
	WMenu *menu;
	int ret;
	
	if(!WTHING_IS(thing, WTHING_MENU))
		return;
	
	menu=(WMenu*)thing;
	
	if(!menu_select_at(menu, ev->x_root, ev->y_root)){
		thing=find_thing(ev->subwindow);
	
		if(thing==(WThing*)menu || thing==NULL ||
		   !WTHING_IS(thing, WTHING_MENU)){
			end_scroll();
			return;
		}
			
		wglobal.grab_holder=thing;
		menu=(WMenu*)thing;
		
		if(!menu_select_at(menu, ev->x_root, ev->y_root)){
			end_scroll();
			return;
		}
	}
	
	if(test_scroll(menu, ev->x_root, ev->y_root))
		start_scroll(menu);
}


/* */


static WFuncClass fclass_menu={
	NULL,
	menu_motion,
	menu_button,
	ARGTYPE_NONE,
};


static WFunction dummy_func={
	&fclass_menu,
	NULL,
	NULL,
	0
};


void init_menus()
{
	WFuncBinder binder={&dummy_func, 0, ARGTYPE_NONE};

	add_binding(ACT_BUTTONPRESS, ACTX_MENU, 0, AnyButton, &binder);
	add_binding(ACT_BUTTONMOTION, ACTX_MENU, 0, AnyButton, &binder);
	add_binding(ACT_BUTTONCLICK, ACTX_MENU, 0, AnyButton, &binder);
	
	binder.func=lookup_func("menu_raisekeep", ARGTYPE_NONE);
	add_binding(ACT_BUTTONPRESS, ACTX_MENUTITLE, 0, AnyButton, &binder);
	binder.func=lookup_func("menu_close", ARGTYPE_NONE);
	add_binding(ACT_BUTTONDBLCLICK, ACTX_MENUTITLE, 0, AnyButton, &binder);
	binder.func=lookup_func("move", ARGTYPE_NONE);
	add_binding(ACT_BUTTONMOTION, ACTX_MENUTITLE, 0, AnyButton, &binder);
}


/*
 * Menu scrolling
 */


static int scrollhoriz=0, scrollvert=0;
static WMenu *scrollmenu=NULL, *scrolltop=NULL;


static void scrollfunc()
{
	WMenu *menu=scrollmenu;
	int dx=scrollhoriz*SCROLL_AMOUNT;
	int dy=scrollvert*SCROLL_AMOUNT;
	Window win;
	
	if(menu->x+dx>0 && scrollhoriz==1)
		dx=-menu->x;
	else if(menu->x+menu->w+dx<SCREEN->width-SCROLL_BORDER && scrollhoriz==-1)
		dx=-(menu->x+menu->w-SCREEN->width+SCROLL_BORDER);

	if(menu->y+dy>0 && scrollvert==1)
		dy=-menu->y;
	else if(menu->y+menu->h+dy<SCREEN->height-SCROLL_BORDER && scrollvert==-1)
		dy=-(menu->y+menu->h-SCREEN->height+SCROLL_BORDER);

	move_menu(scrolltop, dx, dy);
	
	/* pointer root -> dx, dy */
	get_pointer_rootpos(&dx, &dy);

	if(menu_select_at(menu, dx, dy)){
		if(test_scroll(menu, dx, dy))
			return;
	}else if(test_scroll(menu, dx, dy)){
		return;
	}else{
		if(!find_window_at(dx, dy, &win))
			goto end;
		
		menu=(WMenu*)find_thing_t(win, WTHING_MENU);
		
		if(menu==NULL)
			goto end;
		
		if(test_scroll(menu, dx, dy))
			start_scroll(menu);
	}

end:
	end_scroll();
}


static WMenu *topmenu(WMenu *menu)
{
	WMenu *top=menu;
	
	while(top->stack_above!=NULL && WTHING_IS(top->stack_above, WTHING_MENU))
		top=(WMenu*)top->stack_above;
	
	return top;
}


static void start_scroll(WMenu *menu)
{
	scrollmenu=menu;
	scrolltop=topmenu(menu);
	set_timer(SCROLL_DELAY, scrollfunc);
}


static void end_scroll()
{
	if(scrollmenu!=NULL){
		reset_timer();
		scrollmenu=NULL;
	}
}


static bool test_scroll(WMenu *menu, int rx, int ry)
{
	if(rx<=0 && menu->x<0)
		scrollhoriz=1;
	else if(rx>=SCREEN->width-1 &&
			menu->x+menu->w>=SCREEN->width-SCROLL_BORDER)
		scrollhoriz=-1;
	else
		scrollhoriz=0;

	if(ry<=0 && menu->y<0)
		scrollvert=1;
	else if(ry>=SCREEN->height-1 &&
			menu->y+menu->h>=SCREEN->height-SCROLL_BORDER)
		scrollvert=-1;
	else
		scrollvert=0;

	return (scrollhoriz!=0 || scrollvert!=0);
}


/* */


static bool is_vis_entry(WMenu *menu, int entry, int *dxret, int *dyret)
{
	int ey, ey2;
	int dx=0, dy=0;

	/* first entry - show title as well */
	if(entry<=0)
		ey=0;
	else
		ey=menu->title_height+CF_MENU_V_SPACE+entry*menu->entry_height;
		
	if(entry<0)
		ey2=menu->title_height;
	else
		ey2=menu->title_height+CF_MENU_V_SPACE+(entry+1)*menu->entry_height;
	
	/* some padding */
	ey-=menu->entry_height/2;
	ey2+=menu->entry_height/2;
	
	if(menu->x<0){
		dx=-menu->x;
	}else if(menu->x+menu->w>SCREEN->width){
		dx=SCREEN->width-(menu->x+menu->w);
	}

	if(menu->y+ey<0){
		dy=-menu->y-ey;
	}else if(menu->y+ey2>SCREEN->height){
		dy=-(menu->y+ey2-SCREEN->height);
	}
	
	*dxret=dx;
	*dyret=dy;
	
	return (dx==0 && dy==0);
}


static void vis_entry(WMenu *menu, int entry)
{
	int dx, dy;
	
	if(is_vis_entry(menu, entry, &dx, &dy))
		return;
	
	move_menu(topmenu(menu), dx, dy);
}

