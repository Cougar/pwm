/*
 * pwm/signal.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 *
 * You may distribute and modify this program under the terms of either
 * the Clarified Artistic License or the GNU GPL, version 2 or later.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "property.h"
#include "signal.h"
#include "exec.h"


static void (*tmr_handler)()=NULL;
static int kill_sig=0;
static bool had_tmr=0;


void check_signals()
{
	char *tmp=NULL;
	
	if(kill_sig!=0){
		if(kill_sig==SIGUSR1){
			if(wglobal.parent==0)
				tmp=get_string_property(COMM_WIN, wglobal.atom_private_ipc,
										NULL);
			wm_restart_other(tmp);
			assert(0);
		}
	
		if(kill_sig==SIGTERM)
			wm_exit();

		deinit();
		kill(getpid(), kill_sig);
	}

	if(had_tmr){
		had_tmr=FALSE;
		if(tmr_handler!=NULL)
			tmr_handler();
		return;
	}
}


static void do_set_timer(int msecs)
{
	struct itimerval val={{0, 0}, {0, 0}};
	
	val.it_value.tv_usec=msecs*1000;
	val.it_interval.tv_usec=msecs*1000;
	
	setitimer(ITIMER_REAL, &val, NULL);
}


void set_timer(uint msecs, void (*handler)())
{
	/* First disable it */
	do_set_timer(0);
	
	tmr_handler=handler;
	had_tmr=FALSE;
	
	do_set_timer(msecs);
}


void reset_timer()
{
	tmr_handler=NULL;
	
	do_set_timer(0);
}


/* */


static void fatal_signal_handler(int signal_num)
{
	warn("Caught fatal signal %d. Dying without deinit.", signal_num);
	
	signal(signal_num, SIG_DFL);
	kill(getpid(), signal_num);
}

		   
static void deadly_signal_handler(int signal_num)
{
	warn("Caught signal %d. Dying.", signal_num);
	kill_sig=signal_num;
	
	signal(signal_num, SIG_DFL);
}


static void chld_handler(int signal_num)
{
	pid_t pid;
	int i;
	
	while((pid=waitpid(-1, NULL, WNOHANG|WUNTRACED))>0){
		if(wglobal.parent!=0)
			continue;
		if(wglobal.children==NULL)
			continue;
		for(i=0; i<wglobal.n_children; i++){
			if(wglobal.children[i]==pid){
				wglobal.children[i]=0;
				wglobal.n_alive=0;
				break;
			}
		}
	}
}


static void exit_handler(int signal_num)
{
	kill_sig=signal_num;
}


static void timer_handler(int signal_num)
{
	had_tmr=TRUE;
}


static void ignore_handler(int signal_num)
{
	
}


#ifndef SA_RESTART
 /* glibc is broken (?) and does not define SA_RESTART with
  * '-ansi -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED', so just try to live
  * without it.
  */
#warning SA_RESTART not defined
#define SA_RESTART 0
#endif


void trap_signals()
{
	struct sigaction sa;

#define DEADLY(X) signal(X, deadly_signal_handler);
#define FATAL(X) signal(X, fatal_signal_handler);
#define IGNORE(X) signal(X, SIG_IGN)
	
	DEADLY(SIGHUP);
	DEADLY(SIGINT);
	DEADLY(SIGQUIT);
	DEADLY(SIGABRT);

	FATAL(SIGILL);
	FATAL(SIGSEGV);
	FATAL(SIGFPE);
	FATAL(SIGBUS);
	
	IGNORE(SIGTRAP);
	IGNORE(SIGWINCH);

	sigemptyset(&(sa.sa_mask));
	sa.sa_handler=chld_handler;
	sa.sa_flags=SA_NOCLDSTOP|SA_RESTART;
	sigaction(SIGCHLD, &sa, NULL);

	sa.sa_handler=exit_handler;
	sa.sa_flags=SA_RESTART;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	
	sa.sa_handler=timer_handler;
	sigaction(SIGALRM, &sa, NULL);

	/* SIG_IGN is preserved over execve and since the the default action
	 * for SIGPIPE is not to ignore it, some programs may get upset if
	 * the behavior is not the default.
	 */
	sa.sa_handler=ignore_handler;
	sigaction(SIGPIPE, &sa, NULL);
	
	
#undef IGNORE
#undef FATAL
#undef DEADLY
}

