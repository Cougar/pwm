/*
 * pwm/font.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_FONT_H
#define INCLUDED_FONT_H

#include <libtu/types.h>
#include <X11/Xlib.h>

#define FONT_HEIGHT(X) ((X)->ascent+(X)->descent)
#define FONT_BASELINE(X) ((X)->ascent)
#define MAX_FONT_WIDTH(X) ((X)->max_bounds.width)

extern XFontStruct *load_font(Display *dpy, const char *fontname);
extern char *make_label(XFontStruct *fnt, const char *str, const char *trailer,
						int maxw, int *wret);

#endif /* INCLUDED_FONT_H */
