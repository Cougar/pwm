/*
 * pwm/frameid.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_FRAMEID_H
#define INCLUDED_FRAMEID_H

#include "frame.h"

#define FRAME_ID_START_CLIENT 256
#define FRAME_ID_REORDER_INTERVAL 1024

extern WFrame *find_frame_by_id(int id);
extern int use_frame_id(int id);
extern int new_frame_id();
extern void free_frame_id(int id);

#endif /* INCLUDED_FRAMEID_H */
