/*
 * pwm/function.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_FUNCTION_H
#define INCLUDED_FUNCTION_H

#include "common.h"


enum{
	ARGTYPE_NONE,
	ARGTYPE_INT,
	ARGTYPE_STRING,
	ARGTYPE_PTR
};

#define ARG_TO_INT(X) ((X).i)
#define ARG_TO_STRING(X) ((X).str)
#define ARG_TO_P(X) ((X).p)

#define SET_INT_ARG(X, I) ((X).i=(I))
#define SET_STRING_ARG(X, STR) ((X).str=(STR))
#define SET_P_ARG(X, P) ((X).p=(void*)(P))

#define SET_NULL_ARG(X) ((X).p=NULL)
#define IS_NULL_ARG(X) ((X).p==NULL)

#define INIT_NULL_ARG {NULL}

typedef union _WFuncArg{
	void *p;
	char *str;
	int i;
} WFuncArg;


struct _WFunction;


typedef void WFuncHandler(WThing *thing,
						  struct _WFunction *func, WFuncArg arg);
typedef void WButtonHandler(WThing *thing, XButtonEvent *ev,
							struct _WFunction *func, WFuncArg arg);
typedef void WMotionHandler(WThing *thing, XMotionEvent *ev, int dx, int dy,
							struct _WFunction *func, WFuncArg arg);


typedef struct _WFuncClass{
	WFuncHandler *handler;		/* generic */
	WMotionHandler *motion;		/* pointer motion (drag) */
	WButtonHandler *button;		/* button click/press/release */
	int arg_type;
} WFuncClass;


typedef struct _WFunction{
	WFuncClass *fclass;
	const char *fname;
	const void* func;
	int opval;
} WFunction;


typedef struct _WFuncBinder{
	WFunction *func;
	WFuncArg arg;
	int arg_type;
} WFuncBinder;


/* */


extern WFunction *lookup_func(const char *name, int arg_type);


#endif /* INCLUDED_FUNCTION_H */
