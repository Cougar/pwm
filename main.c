/*
 * pwm/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libtu/util.h>
#include <libtu/optparser.h>

#include <X11/Xlib.h>

#include "common.h"
#include "screen.h"
#include "config.h"
#include "event.h"
#include "cursor.h"
#include "signal.h"
#include "binding.h"
#include "readconfig.h"
#include "winlist.h"


static bool initialize(const char *display, const char *cfgfile,
					   bool onescreen);

static pid_t fork_screen(int scr);


/* */


WGlobal wglobal;


/* Options. Getopt is not used because getopt_long is quite gnu-specific
 * and they don't know of '-display foo' -style args anyway.
 * Instead, I've reinvented the wheel in libtu :(.
 */
static OptParserOpt opts[]={
	{OPT_ID('d'), 	"display", 	OPT_ARG, "host:dpy.scr", "X display to use"},
	{'c', 			"cfgfile", 	OPT_ARG, "config_file", "Configuration file"},
	{OPT_ID('o'), 	"onescreen", 0,		 NULL, "Manage default screen only"},
	{0, NULL, 0, NULL, NULL}
};


static const char wm_usage_tmpl[]=
	"Usage: $p [options]\n\n$o\n";


static const char wm_about[]=
	"PWM " PWM_VERSION ", copyright (c) Tuomo Valkonen 1999-2001.\n"
	"This program may be copied and modified under the terms of the "
	"Artistic License or the GNU GPL.\n";


static OptParserCommonInfo wm_cinfo={
	PWM_VERSION,
	wm_usage_tmpl,
	wm_about
};


	
/* */


int main(int argc, char*argv[])
{
	int opt;
	const char *cfgfile=NULL;
	bool onescreen=FALSE;
	
	libtu_init(argv[0]);
	
	wglobal.argc=argc;
	wglobal.argv=argv;
	wglobal.dpy=NULL;
	wglobal.display=NULL;
	wglobal.current_winobj=NULL;
	wglobal.previous_winobj=NULL;
	wglobal.grab_holder=NULL;
	wglobal.input_mode=INPUT_NORMAL;
	wglobal.dblclick_delay=CF_DBLCLICK_DELAY;
	wglobal.parent=0;
	wglobal.n_children=0;
	/* The rest don't need to be initialized here */

	optparser_init(argc, argv, OPTP_MIDLONG, opts, &wm_cinfo);
	
	while((opt=optparser_get_opt())){
		switch(opt){
		case OPT_ID('d'):
			wglobal.display=optparser_get_arg();
			break;
		case 'c':
			cfgfile=optparser_get_arg();
			break;
		case OPT_ID('o'):
			onescreen=TRUE;
			break;
		default:
			optparser_print_error();
			return EXIT_FAILURE;
		}
	}

	if(!initialize(wglobal.display, cfgfile, onescreen)){
		if(wglobal.parent!=0){
			deinit();
		}else{
			/* Default screen failed but the others may be alive and need
			 * this process for exit/restart communication.
			 */
			while(wglobal.n_alive>0){
				pause();
				check_signals();
			}
		}
		return EXIT_FAILURE;
	}
	
	mainloop();
	
	/* The code should never return here */
	return EXIT_SUCCESS;
}


/* */


static bool initialize(const char*display, const char *cfgfile,
					   bool onescreen)
{
	Display *dpy;
	int scr, i, nscr;
	
	/* Open the display. */
	dpy=XOpenDisplay(display);
	
	if(dpy==NULL)
		die("Could not connect to X display '%s'", XDisplayName(display));

	scr=DefaultScreen(dpy);
	nscr=ScreenCount(dpy);

	trap_signals();
	
	if(!onescreen && nscr>1){
		/* Managing multiple displays. Fork a new process for each additional
		 * screen. */
		
		/* Close the display... */
		XCloseDisplay(dpy);
		
		wglobal.children=ALLOC_N(pid_t, nscr);
		
		if(wglobal.children==NULL)
			die_err();
		
		wglobal.n_children=nscr;
		wglobal.n_alive=0;
		
		for(i=0; i<nscr; i++){
			if(i==scr)
				continue;
			if(fork_screen(i)==0){
				scr=i;
				break;
			}
		}
		
		/* ... and reopen it for each process. */
		dpy=XOpenDisplay(display);
		if(dpy==NULL)
			die("Could not connect to X display %s", XDisplayName(display));
	}

	/* Initialize */
	wglobal.dpy=dpy;
	wglobal.current_winobj=NULL;

	wglobal.conn=ConnectionNumber(dpy);
	wglobal.win_context=XUniqueContext();
	
	wglobal.atom_wm_state=XInternAtom(dpy, "WM_STATE", False);
	wglobal.atom_wm_change_state=XInternAtom(dpy, "WM_CHANGE_STATE", False);
	wglobal.atom_wm_protocols=XInternAtom(dpy, "WM_PROTOCOLS", False);
	wglobal.atom_wm_delete=XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wglobal.atom_wm_take_focus=XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	wglobal.atom_wm_colormaps=XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
	wglobal.atom_frame_id=XInternAtom(dpy, "_PWM_FRAME_ID", False);
	wglobal.atom_workspace_num=XInternAtom(dpy, "_PWM_WORKSPACE_NUM", False);
	wglobal.atom_workspace_info=XInternAtom(dpy, "_PWM_WORKSPACE_INFO", False);
	wglobal.atom_private_ipc=XInternAtom(dpy, "_PWM_PRIVATE_IPC", False);
#ifndef CF_NO_MWM_HINTS	
	wglobal.atom_mwm_hints=XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
#endif
	
	init_bindings();
	init_menus();
	load_cursors();	
	register_winlist();
	
	if(!preinit_screen(scr))
		return FALSE;

	read_config(cfgfile);
	postinit_screen();

	/*atexit(deinit);*/
	
	return TRUE;
}


static pid_t fork_screen(int scr)
{
	pid_t pid;
	
	pid=fork();
	
	if(pid==0){
		wglobal.parent=getppid();
	}else if(pid<0){
		warn("Failed to fork process for screen %d: %s",
			 scr, strerror(errno));
	}else{
		wglobal.children[scr]=pid;
		wglobal.n_alive++;
	}
	
	return pid;
}


/* */


void deinit()
{
	Display *dpy;
	
	if(wglobal.dpy==NULL)
		return;
	
	deinit_screen();
	
	dpy=wglobal.dpy;
	wglobal.dpy=NULL;
	
	XCloseDisplay(dpy);
}
