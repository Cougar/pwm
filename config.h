/*
 * pwm/config.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_CONFIG_H
#define INCLUDED_CONFIG_H

#include "version.h"


/* Behaviour-controlling booleans.
 *
 * CF_NO_WILD_WINDOWS
 *  Disable "wild windows". Windows that request no border and title
 *  via MWM decoration hints are considered wild. They get no
 *  decorations and are even allowed to move themselves.
 *
 * CF_NO_LOCK_HACK
 *  Disable hack to ignore states of evil locking modifier keys.
 *
 * CF_NO_MWM_HINTS
 *  Ignore MWM hints.
 *
 * CF_NO_AUTOFOCUS
 *  Don't autofocus new transients of current window/new windows
 *  when there is no current window.
 *
 * CF_AUTOFOCUS_ALL
 *  Autofocus all new windows
 *
 * CF_CWIN_TOPLEFT
 *  Place small windows in multi-window frames at top left corner
 *  rather than center.
 *
 * CF_IGNORE_NONTRANSIENT_LOCATION
 *  Ignore client supplied window location for all but transient
 *  windows.
 *
 * CF_FLACCID_PLACEMENT_UDLR
 *  Place windows from top to bottom instead of left to right.
 * 
 * CF_PACK_MOVE
 *  Enable pack_move function
 * 
 * CF_GOTODIR
 *  Enable gotodir function
 * 
 * CF_NO_NUMBERING
 *  Do not number windows with the same name (e.g xterm<2>)
 * 
 */

/*#define CF_NO_WILD_WINDOWS*/
/*#define CF_NO_LOCK_HACK*/
/*#define CF_NO_MWM_HINTS*/
/*#define CF_NO_AUTOFOCUS*/
/*#define CF_AUTOFOCUS_ALL*/
/*#define CF_CWIN_TOPLEFT*/
/*#define CF_IGNORE_NONTRANSIENT_LOCATION*/
/*#define CF_NO_NUMBERING*/
#define CF_FLACCID_PLACEMENT_UDLR
#define CF_PACK_MOVE
#define CF_GOTODIR


/* Don't modify these
 */

#ifndef PWM_VERSION
#define PWM_VERSION ""
#endif

#ifndef ETCDIR
#define ETCDIR "/etc"
#endif

#define CF_SYS_CONFIG_LOCATION ETCDIR"/pwm/"
#define CF_GLOBAL_CFGFILE CF_SYS_CONFIG_LOCATION"pwm.conf"
#define CF_USER_CONFIG_LOCATION ".pwm/"
#define CF_USER_CFGFILE CF_USER_CONFIG_LOCATION"pwm.conf"

#define GRDATA (&(wglobal.grdata))
#define SCREEN (&(wglobal.screen))

#define CF_FONT (GRDATA->font)
#define CF_MENU_FONT (GRDATA->menu_font)

#define CF_BORDER_WIDTH (GRDATA->border_width)
#define CF_BEVEL_WIDTH (GRDATA->bevel_width)

#define CF_BAR_MIN_WIDTH (GRDATA->bar_min_width)
#define CF_BAR_MAX_WIDTH_Q (GRDATA->bar_max_width_q)
#define CF_TAB_MIN_WIDTH (GRDATA->tab_min_width)

#define	CF_AUTORAISE_TIME (GRDATA->autoraise_time)

/* Drawing
 */

#define CF_TAB_TEXT_Y_PAD 2
#define CF_TAB_TEXT_MIN_X_PAD 2
#define CF_TAB_TEXT_MAX_X_PAD 20

#define CF_TAB_TEXT_Y_OFF (CF_BEVEL_WIDTH+CF_TAB_TEXT_Y_PAD)

#define CF_TAB_MIN_TEXT_X_OFF (CF_BEVEL_WIDTH+CF_TAB_TEXT_MIN_X_PAD)
#define CF_TAB_MAX_TEXT_X_OFF (CF_BEVEL_WIDTH+CF_TAB_TEXT_MAX_X_PAD)

#define CF_MENUTITLE_H_PAD 3
#define CF_MENUENT_H_PAD 3
#define CF_MENUTITLE_V_PAD CF_TAB_TEXT_Y_PAD
#define CF_MENUENT_V_PAD 3

#define CF_MENUTITLE_H_SPACE (CF_MENUTITLE_H_PAD+CF_BEVEL_WIDTH)
#define CF_MENUENT_H_SPACE (CF_MENUENT_H_PAD+CF_BEVEL_WIDTH)
#define CF_MENUTITLE_V_SPACE (CF_MENUTITLE_V_PAD+CF_BEVEL_WIDTH)
#define CF_MENUENT_V_SPACE CF_MENUENT_V_PAD
#define CF_MENU_V_SPACE CF_BEVEL_WIDTH
#define CF_SUBMENU_IND_H_SPACE	3

#define CF_WANT_TRANSPARENT_TERMS

/* Sizes and locations
 */

#define CF_WIN_MIN_WIDTH 8
#define CF_WIN_MIN_HEIGHT 8

#define CF_MAX_MOVERES_STR_SIZE 32
#define CF_MOVERES_WIN_X 5
#define CF_MOVERES_WIN_Y 5

#define CF_DRAG_TRESHOLD 2
#define CF_EDGE_RESISTANCE 16
#define CF_STEP_SIZE 16
#define CF_CORNER_SIZE (16+8)


/* Cursors
 */

#define CF_CURSOR_DEFAULT XC_left_ptr
#define CF_CURSOR_RESIZE XC_sizing
#define CF_CURSOR_MOVE XC_fleur
#define CF_CURSOR_DRAG XC_cross


/* Defaults
 */

#define CF_DBLCLICK_DELAY 250
#define CF_DEFAULT_N_WORKSPACES	6
#define CF_FALLBACK_FONT_NAME "fixed"


/* Menu scrolling
 */

#define SCROLL_DELAY 8
#define SCROLL_AMOUNT 4
#define SCROLL_BORDER 4

#endif /* INCLUDED_CONFIG_H */
