/*
 * libtu/output.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2000. 
 * See the included file LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <string.h>

#include <libtu/misc.h>
#include <libtu/output.h>
#include <libtu/util.h>

#if !defined(LIBTU_NO_ERRMSG) && !defined(HAS_SYSTEM_ASPRINTF)
#include "../snprintf_2.2/snprintf.h"
#endif


/* verbose
 */

static bool verbose_mode=FALSE;
static int verbose_indent_lvl=0;
static bool progname_enable=TRUE;

#define INDENTATOR_LENGTH 4

static char indentator[]={' ', ' ', ' ', ' '};


void verbose(const char *p, ...)
{
	va_list args;
	
	va_start(args, p);
	
	verbose_v(p, args);
	
	va_end(args);
}
		   

void verbose_v(const char *p, va_list args)
{
	int i;
	
	if(verbose_mode){
		for(i=0; i<verbose_indent_lvl; i++)
			writef(stdout, indentator, INDENTATOR_LENGTH);
		
		vprintf(p, args);
		fflush(stdout);
	}
}


void verbose_enable(bool enable)
{
	verbose_mode=enable;
}


int verbose_indent(int depth)
{
	int old=verbose_indent_lvl;
	
	if(depth>=0)
		verbose_indent_lvl=depth;
	
	return old;
}
		

void warn_progname_enable(bool enable)
{
	progname_enable=enable;
}


static void put_prog_name()
{
	const char*progname;
	
	if(!progname_enable)
		return;
	
	progname=prog_execname();
	
	if(progname==NULL)
		return;
	
	fprintf(stderr, "%s: ", (char*)progname);
}

/* warn
 */

#define CALL_V(NAME, ARGS) \
	va_list args; va_start(args, p); NAME ARGS; va_end(args);


#ifndef LIBTU_NO_ERRMSG

void libtu_asprintf(char **ret, const char *p, ...)
{
	CALL_V(vasprintf, (ret, p, args));
}


void libtu_vasprintf(char **ret, const char *p, va_list args)
{
	vasprintf(ret, p, args);
}

#endif

void warn(const char *p, ...)
{
	CALL_V(warn_v, (p, args));
}


void warn_obj(const char *obj, const char *p, ...)
{
	CALL_V(warn_obj_v, (obj, p, args));
}


void warn_obj_line(const char *obj, int line, const char *p, ...)
{
	CALL_V(warn_obj_line_v, (obj, line, p, args));
}


void warn_obj_v(const char *obj, const char *p, va_list args)
{
	warn_obj_line_v(obj, -1, p, args);
}


void warn_v(const char *p, va_list args)
{
	put_prog_name();
	vfprintf(stderr, p, args);
	putc('\n', stderr);
}


void warn_obj_line_v(const char *obj, int line, const char *p, va_list args)
{
	put_prog_name();
	if(obj!=NULL){
		if(line>0)
			fprintf(stderr, TR("%s:%d: "), obj, line);
		else		
			fprintf(stderr, "%s: ", obj);
	}else{
		if(line>0)
			fprintf(stderr, TR("%d: "), line);
	}
	vfprintf(stderr, p, args);
	putc('\n', stderr);
}


void warn_err()
{
	put_prog_name();
	fprintf(stderr, "%s\n", strerror(errno));
}


void warn_err_obj(const char *obj)
{
	put_prog_name();
	if(obj!=NULL)
		fprintf(stderr, "%s: %s\n", obj, strerror(errno));
	else
		fprintf(stderr, "%s\n", strerror(errno));
}

void warn_err_obj_line(const char *obj, int line)
{
	put_prog_name();
	if(obj!=NULL){
		if(line>0)
			fprintf(stderr, TR("%s:%d: %s\n"), obj, line, strerror(errno));
		else
			fprintf(stderr, "%s: %s\n", obj, strerror(errno));
	}else{
		if(line>0)
			fprintf(stderr, TR("%d: %s\n"), line, strerror(errno));
		else
			fprintf(stderr, TR("%s\n"), strerror(errno));
	}

}


/* errmsg
 */
#ifndef LIBTU_NO_ERRMSG

#define CALL_V_RET(NAME, ARGS) \
	char *ret; va_list args; va_start(args, p); ret=NAME ARGS; va_end(args); return ret;


char* errmsg(const char *p, ...)
{
	CALL_V_RET(errmsg_v, (p, args));
}


char *errmsg_obj(const char *obj, const char *p, ...)
{
	CALL_V_RET(errmsg_obj_v, (obj, p, args));
}


char *errmsg_obj_line(const char *obj, int line, const char *p, ...)
{
	CALL_V_RET(errmsg_obj_line_v, (obj, line, p, args));
}


char* errmsg_obj_v(const char *obj, const char *p, va_list args)
{
	return errmsg_obj_line_v(obj, -1, p, args);
}


char *errmsg_v(const char *p, va_list args)
{
	char *res;
	vasprintf(&res, p, args);
	return res;
}


char *errmsg_obj_line_v(const char *obj, int line, const char *p, va_list args)
{
	char *res1=NULL, *res2, *res3;
	if(obj!=NULL){
		if(line>0)
			asprintf(&res1, TR("%s:%d: "), obj, line);
		else		
			asprintf(&res1, "%s: ", obj);
	}else{
		if(line>0)
			asprintf(&res1, TR("%d: "), line);
	}
	asprintf(&res2, p, args);
	if(res1!=NULL){
		if(res2==NULL)
			return NULL;
		res3=scat(res1, res2);
		free(res1);
		free(res2);
		return res3;
	}
	return res2;
}


char *errmsg_err()
{
	char *res;
	asprintf(&res, "%s\n", strerror(errno));
	return res;
}


char *errmsg_err_obj(const char *obj)
{
	char *res;
	if(obj!=NULL)
		asprintf(&res, "%s: %s\n", obj, strerror(errno));
	else
		asprintf(&res, "%s\n", strerror(errno));
	return res;
}


char *errmsg_err_obj_line(const char *obj, int line)
{
	char *res;
	if(obj!=NULL){
		if(line>0)
			asprintf(&res, TR("%s:%d: %s\n"), obj, line, strerror(errno));
		else
			asprintf(&res, "%s: %s\n", obj, strerror(errno));
	}else{
		if(line>0)
			asprintf(&res, TR("%d: %s\n"), line, strerror(errno));
		else
			asprintf(&res, TR("%s\n"), strerror(errno));
	}
	return res;
}

#endif /* LIBTU_NO_ERRMSG */


/* die
 */

void die(const char *p, ...)
{
	CALL_V(die_v, (p, args));
}


void die_v(const char *p, va_list args)
{
	warn_v(p, args);
	exit(EXIT_FAILURE);
}


void die_obj(const char *obj, const char *p, ...)
{
	CALL_V(die_obj_v, (obj, p, args));
}


void die_obj_line(const char *obj, int line, const char *p, ...)
{
	CALL_V(die_obj_line_v, (obj, line, p, args));
}


void die_obj_v(const char *obj, const char *p, va_list args)
{
	warn_obj_v(obj, p, args);
	exit(EXIT_FAILURE);
}


void die_obj_line_v(const char *obj, int line, const char *p, va_list args)
{
	warn_obj_line_v(obj, line, p, args);
	exit(EXIT_FAILURE);
}


void die_err()
{
	warn_err();
	exit(EXIT_FAILURE);
}


void die_err_obj(const char *obj)
{
	warn_err_obj(obj);
	exit(EXIT_FAILURE);
}


void die_err_obj_line(const char *obj, int line)
{
	warn_err_obj_line(obj, line);
	exit(EXIT_FAILURE);
}
