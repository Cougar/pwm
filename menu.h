/*
 * pwm/menu.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_MENU_H
#define INCLUDED_MENU_H

#include "common.h"
#include "thing.h"
#include "winobj.h"
#include "function.h"


#define WMENU_KEEP			0x0001
#define WMENU_CONTEXTUAL	0x0002
#define WMENU_NOTITLE		0x0004

/*
#define WMENUENT_CHECKABLE	0x0001
#define WMENUENT_CHECKED	0x0002
#define WMENUENT_DISABLED	0x0004
 */
#define WMENUENT_SUBMENU	0x0008

#define WMENUDATA_CONTEXTUAL 0x0001
#define WMENUDATA_USER		0x0002


enum{
	MENU_CMD_NEXT,
	MENU_CMD_PREV,
	MENU_CMD_ENTERSUB,
	MENU_CMD_LEAVESUB,
	MENU_CMD_CLOSE,
	MENU_CMD_RAISEKEEP,
	MENU_CMD_KEEP,
	MENU_CMD_EXECUTE
};

typedef struct _WMenuEnt{
	char *name;
	int flags;
	union _U{
		struct _F{
			WFunction *function;
			WFuncArg arg;
			int arg_type;
		} f;
		struct _Winlist{
			WClientWin *clientwin;
			int sort;
		} winlist;
		struct _WMenuData *submenu;
		int num;
		void *other;
	} u;
} WMenuEnt;


typedef struct _WMenuData{
	char *title;
	char *name;
	int flags;
	bool (*init_func)(struct _WMenuData *data, WThing *context);
	void (*exec_func)(struct _WMenuEnt *entry, WThing *context);
	void (*deinit_func)(struct _WMenuData *data);
	int nentries;
	WMenuEnt *entries;
	int nref;
	struct _WMenu *inst1;
	struct _WMenuData *menudata_next, *menudata_prev;
} WMenuData;


typedef struct _WMenu{
	INHERIT_WWINOBJ;
	
	Window menu_win;
	
	WMenuData *data;
	int selected;
	
	int title_height;
	int entry_height;

	WThing *context;
} WMenu;

#define MENU_SUBMENU(MENU) ((WMenu*)(MENU)->stack_above_list)
#define MENU_PARENT(MENU) ((WMenu*)(MENU)->stack_above)
#define SUBMENU_DATA(ENT) ((ENT)->u.submenu)

extern void init_menus();

extern void show_menu(WMenuData *mdata, WThing *context, int x, int y,
					  bool button_down);

extern void menu_command(WMenu *menu, int cmd);

extern void activate_menu(WMenu *menu);
extern void deactivate_menu(WMenu *menu);

extern void destroy_menu_tree(WMenu *menu);
extern void destroy_contextual_menus();

extern void set_menu_pos(WMenu *menu, int x, int y);

extern void update_menus(WMenuData *mdata);

extern bool register_menudata(WMenuData *mdata);
extern void unregister_menudata(WMenuData *mdata);
extern void free_user_menudata(WMenuData *mdata);
extern WMenuData *lookup_menudata(const char *name);
extern void free_menu_entries(WMenuEnt *ents, int n);

#endif /* INCLUDED_MENU_H */
