/*
 * pwm/moveres.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_MOVERES_H
#define INCLUDED_MOVERES_H

#include "common.h"
#include "frame.h"

#define MOVERES_RIGHT 	0x01
#define MOVERES_LEFT	0x02
#define MOVERES_BOTTOM	0x10
#define MOVERES_TOP	 	0x20
#define RESIZE_WDEC		0x04
#define RESIZE_HDEC		0x40

extern void resize_frame(WFrame *frame, int dx, int dy, int mode,
						 int stepsize);
extern void resize_frame_end(WFrame *frame);
extern void move_winobj(WWinObj *frame, int dx, int dy, int stepsize);
extern void move_winobj_end(WWinObj *frame);

extern void keyboard_move(WWinObj *obj, int mask);
extern void keyboard_move_stepped(WWinObj *obj, int mask);
extern void keyboard_resize(WFrame *frame, int mask);
extern void keyboard_resize_stepped(WFrame *frame, int mask);
extern void keyboard_moveres_begin(WWinObj *obj);
extern void keyboard_moveres_end(WWinObj *obj);
extern void keyboard_moveres_cancel(WWinObj *obj);

extern void correct_aspect(int max_w, int max_h, XSizeHints *hints,
						   int *wret, int *hret);
extern void calc_size(WFrame *frame, WClientWin *cwin, bool fmode,
					  int *wr, int *hr);

extern void pack_move(WWinObj *obj, int mask);
extern void gotodir(WWinObj *obj, int mask);

#endif /* INCLUDED_MOVERES_H */
