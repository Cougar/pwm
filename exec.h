/*
 * pwm/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#ifndef INCLUDED_EXEC_H
#define INCLUDED_EXEC_H

#include "common.h"

extern void wm_exec(const char *cmd);
extern void wm_restart_other(const char *cmd);
extern void wm_restart();
extern void wm_exit();
extern void wm_exitret();
extern void setup_environ(int scr);

#endif /* INCLUDED_EXEC_H */
