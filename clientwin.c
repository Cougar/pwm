/*
 * pwm/clientwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "clientwin.h"
#include "frame.h"
#include "frameid.h"
#include "property.h"
#include "draw.h"
#include "event.h"
#include "dock.h"
#include "placement.h"
#include "focus.h"
#include "winlist.h"
#include "mwmhints.h"
#include "winprops.h"


static void set_clientwin_state(WClientWin *cwin, int state);

/* */


void get_clientwin_size_hints(WClientWin *cwin)
{
	XSizeHints *hints;
	WClientWin *cwin2;
	int minh, minw;
	long supplied=0;
	
	hints=&(cwin->size_hints);
	
	memset(hints, 0, sizeof(*hints));
	XGetWMNormalHints(wglobal.dpy, cwin->client_win, hints, &supplied);

	if(!(hints->flags&PMinSize) || hints->min_width<CF_WIN_MIN_WIDTH)
		hints->min_width=CF_WIN_MIN_WIDTH;
	if(!(hints->flags&PBaseSize) || hints->base_width<hints->min_width)
		hints->base_width=hints->min_width;

	if(!(hints->flags&PMinSize) || hints->min_height<CF_WIN_MIN_WIDTH)
		hints->min_height=CF_WIN_MIN_WIDTH;
	if(!(hints->flags&PBaseSize) || hints->base_height<hints->min_height)
		hints->base_height=hints->min_height;

	if(hints->flags&PMaxSize){
		if(hints->max_width<hints->base_width)
			hints->max_width=hints->base_width;
		if(hints->max_height<hints->base_height)
			hints->max_height=hints->base_height;
	}
	
	hints->flags|=PBaseSize;/*|PMinSize*/;

	if(hints->flags&PResizeInc){
		if(hints->width_inc<=0 || hints->height_inc<=0){
			warn("Invalid client-supplied width/height increment");
			hints->flags&=~PResizeInc;
		}
	}
	
	if(hints->flags&PAspect){
		if(hints->min_aspect.x<=0 || hints->min_aspect.y<=0 ||
		   hints->min_aspect.x<=0 || hints->min_aspect.y<=0){
			warn("Invalid client-supplied aspect-ratio");
			hints->flags&=~PAspect;
		}
	}
	
	if(!(hints->flags&PWinGravity))
		hints->win_gravity=ForgetGravity;

	if(CWIN_HAS_FRAME(cwin))
		frame_recalc_minmax(CWIN_FRAME(cwin));
}


static void limit_size(WClientWin *cwin, int *w, int *h)
{
	if(cwin->size_hints.flags&PMinSize){
		if(*w<cwin->size_hints.min_width)
			*w=cwin->size_hints.min_width;
		if(*h<cwin->size_hints.min_height)
			*h=cwin->size_hints.min_height;
	}
	
	if(cwin->size_hints.flags&PMaxSize){
		if(*w>cwin->size_hints.max_width)
			*w=cwin->size_hints.max_width;
		if(*h>cwin->size_hints.max_height)
			*h=cwin->size_hints.max_height;
	}
}


static void get_protocols(WClientWin *cwin)
{
	Atom *protocols=NULL, *p;
	int n;
	
	if(!XGetWMProtocols(wglobal.dpy, cwin->client_win, &protocols, &n))
		return;
	
	for(p=protocols; n; n--, p++){
		if(*p==wglobal.atom_wm_delete)
			cwin->flags|=CWIN_P_WM_DELETE;
		else if(*p==wglobal.atom_wm_take_focus)
			cwin->flags|=CWIN_P_WM_TAKE_FOCUS;
	}
	
	if(protocols!=NULL)
		XFree((char*)protocols);
}


static void get_initial_winprops(Window win, int *frame_id_ret, int *ws_ret,
								 int *flags)
{
	int ws=-1, frame_id=0;
	WWinProp *winprop;
	
	winprop=find_winprop_win(win);
	
	if(winprop!=NULL){
		ws=winprop->init_workspace;
		frame_id=winprop->init_frame;
		if(winprop->wildmode==WILDMODE_YES)
			*flags|=CWIN_WILD;
		else if(winprop->wildmode==WILDMODE_NO)
			*flags&=~CWIN_WILD;
	}

	get_integer_property(win, wglobal.atom_workspace_num, &ws);
	get_integer_property(win, wglobal.atom_frame_id, &frame_id);
	
	if(ws<WORKSPACE_STICKY || ws>=SCREEN->n_workspaces)
		ws=WORKSPACE_CURRENT;
	
	*frame_id_ret=frame_id;
	*ws_ret=ws;
}


static void get_colormaps(WClientWin *cwin)
{
	Window *wins;
	XWindowAttributes attr;
	int i, n;
	
	n=do_get_property(wglobal.dpy, cwin->client_win, wglobal.atom_wm_colormaps,
					  XA_WINDOW, 100L, (uchar**)&wins);
	
	if(cwin->n_cmapwins!=0){
		free(cwin->cmapwins);
		free(cwin->cmaps);
	}
	
	if(n>0){
		cwin->cmaps=ALLOC_N(Colormap, n);
		
		if(cwin->cmaps==NULL){
			n=0;
			free(wins);
		}
	}
		
	if(n<=0){
		cwin->cmapwins=NULL;
		cwin->cmaps=NULL;
		cwin->n_cmapwins=0;
		return;
	}
	
	cwin->cmapwins=wins;
	cwin->n_cmapwins=n;
	
	for(i=0; i<n; i++){
		if(wins[i]==cwin->client_win){
			cwin->cmaps[i]=cwin->cmap;
		}else{
			XSelectInput(wglobal.dpy, wins[i], ColormapChangeMask);
			XGetWindowAttributes(wglobal.dpy, wins[i], &attr);
			cwin->cmaps[i]=attr.colormap;
		}
	}
}


static void clear_colormaps(WClientWin *cwin)
{
	int i;
	
	if(cwin->n_cmapwins==0)
		return;
	
	for(i=0; i<cwin->n_cmapwins; i++)
		XSelectInput(wglobal.dpy, cwin->cmapwins[i], 0);
	
	free(cwin->cmapwins);
	free(cwin->cmaps);
}


void install_cmap(Colormap cmap)
{
	if(cmap==None)
		cmap=SCREEN->default_cmap;
	XInstallColormap(wglobal.dpy, cmap);
}


void install_cwin_cmap(WClientWin *cwin)
{
	int i;
	bool found=FALSE;
	/*WClientWin *cwin2;*/

again:
	
	for(i=cwin->n_cmapwins-1; i>=0; i--){
		install_cmap(cwin->cmaps[i]);
		if(cwin->cmapwins[i]==cwin->client_win)
			found=TRUE;
	}
	
	if(found)
		return;
	
	/*if(cwin->transient_for!=None){
		cwin2=find_clientwin(cwin->transient_for);
		if(cwin2!=NULL){
			cwin=cwin2;
			goto again;
		}
	}*/
	
	install_cmap(cwin->cmap);
}


/* */

/* Not all gravities (Center, Static) are supported as they are mostly
 * unused
 */

static void account_gravity(int frameflags, WClientWin *cwin, int *x, int *y)
{
	int b=(frameflags&WFRAME_NO_BORDER ? 0 : CF_BORDER_WIDTH*2);
	int grav=cwin->size_hints.win_gravity;
	
	if(grav==EastGravity || grav==SouthEastGravity || grav==NorthEastGravity)
		*x-=b-cwin->orig_bw*2;
	
	if(grav==SouthGravity || grav==SouthWestGravity || grav==SouthEastGravity){
		*y-=b-cwin->orig_bw*2;
		*y-=(frameflags&WFRAME_NO_BAR ? 0 : GRDATA->bar_height);
	}
}


static void restore_gravity(WFrame *frame, WClientWin *cwin,
							int *xret, int *yret)
{
	int grav=cwin->size_hints.win_gravity;
	
	*xret=frame->x;
	*yret=frame->y;
	
	if(grav==EastGravity || grav==SouthEastGravity || grav==NorthEastGravity)
		*xret+=frame->frame_ix*2-cwin->orig_bw*2;

	if(grav==SouthGravity || grav==SouthWestGravity || grav==SouthEastGravity)
		*yret+=frame->frame_iy*2-cwin->orig_bw*2+frame->bar_h;
}


/* */


static void configure_cwin_bw(Window win, int bw)
{
	XWindowChanges wc;
	ulong wcmask=CWBorderWidth;
	
	wc.border_width=bw;
	XConfigureWindow(wglobal.dpy, win, wcmask, &wc);
}


WClientWin* create_clientwin(Window win, int flags, int initial_state,
							 const XWindowAttributes *attr)
{
	WClientWin *cwin;
	
	cwin=ALLOC(WClientWin);
	
	if(cwin==NULL){
		warn_err();
		return NULL;
	}
	
	WTHING_INIT(cwin, WTHING_CLIENTWIN);

	cwin->flags=flags;
	cwin->client_win=win;
		
	cwin->orig_bw=attr->border_width;
	cwin->client_h=attr->height;
	cwin->client_w=attr->width;
	cwin->cmap=attr->colormap;
	
	cwin->n_cmapwins=0;
	get_colormaps(cwin);
	get_clientwin_size_hints(cwin);
	get_protocols(cwin);
	limit_size(cwin, &(cwin->client_w), &(cwin->client_h));

	cwin->state=initial_state;
	cwin->name=get_string_property(cwin->client_win, XA_WM_NAME, NULL);
	cwin->icon_name=get_string_property(cwin->client_win,
										XA_WM_ICON_NAME, NULL);
	cwin->label=NULL;
	cwin->label_width=0;
	cwin->label_next=cwin;
	cwin->label_prev=cwin;
	cwin->label_inst=0;
	
	cwin->event_mask=CLIENT_MASK;
	cwin->transient_for=None;
	cwin->dockpos=INT_MAX;
	
	/* Unnecessary, done when reparenting */
	/* XSelectInput(wglobal.dpy, win, cwin->event_mask); */

	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)cwin);
	XAddToSaveSet(wglobal.dpy, win);

	LINK_ITEM(SCREEN->clientwin_list, cwin, s_cwin_next, s_cwin_prev);
	SCREEN->n_clientwin++;
	
	if(cwin->orig_bw!=0)
		configure_cwin_bw(win, 0);

	clientwin_use_label(cwin);
	
	return cwin;
}


static WFrame *find_win_frame(Window win)
{
	WClientWin *cwin=find_clientwin(win);
	
	if(cwin!=NULL && CWIN_HAS_FRAME(cwin))
		return CWIN_FRAME(cwin);
	
	return NULL;
}


static bool add_clientwin(WClientWin *cwin, XWindowAttributes *attr,
						  int mflags)
{
	WFrame *frame=NULL, *above=NULL;
	int frame_id=0, state=NormalState, frameflags=0;
	int ws=-1;
	int w, h;
	Window win=cwin->client_win;
	
	if(XGetTransientForHint(wglobal.dpy, win, &(cwin->transient_for))){
		above=find_win_frame(cwin->transient_for);
		if(above==NULL)
			cwin->transient_for=None;
	}
	
#ifndef CF_NO_MWM_HINTS
	get_mwm_hints(win, &(cwin->flags));
#endif

	get_initial_winprops(win, &frame_id, &ws, &(cwin->flags));

#ifndef CF_NO_WILD_WINDOWS
	if(CWIN_IS_WILD(cwin)){
		frameflags|=(WFRAME_NO_BORDER|WFRAME_NO_BAR);
		mflags|=MANAGE_RESPECT_POS;
	}
#endif
	
	if(frame_id!=0)
		frame=find_frame_by_id(frame_id);
	
	if(frame==NULL){
		if((attr->x!=0 || attr->y!=0 ||
			cwin->size_hints.win_gravity!=ForgetGravity)
#ifdef CF_IGNORE_NONTRANSIENT_LOCATION
		   && cwin->transient_for!=None
#endif
		   ){
			mflags|=MANAGE_RESPECT_POS;
		}
		
		if(!(mflags&MANAGE_RESPECT_POS)){
			/* Find where to place this frame */
			clientwin_to_frame_size(attr->width, attr->height,
									frameflags, &w, &h);
			calc_placement(w, h, ws, &(attr->x), &(attr->y));
		}else{
			account_gravity(frameflags, cwin, &(attr->x), &(attr->y));
		}
		
		frame=create_frame(attr->x, attr->y, attr->width, attr->height,
						   frame_id, frameflags);

		if(frame==NULL)
			return FALSE;
		
		if(above==NULL)
			add_winobj((WWinObj*)frame, ws, LVL_NORMAL);
		else
			add_winobj_above((WWinObj*)frame, (WWinObj*)above);
	}

	/* Actually, this will always succeed */
	if(!frame_attach_clientwin(frame, cwin))
		return FALSE;
	
	set_frame_state(frame, frame->flags&~WFRAME_HIDDEN);

#ifndef CF_NO_AUTOFOCUS
	if(!(mflags&MANAGE_INITIAL) &&
#ifndef CF_AUTOFOCUS_ALL	
	wglobal.current_winobj==(WWinObj*)above &&
#endif	
	on_current_workspace((WWinObj*)frame)){
		if(frame->current_cwin!=cwin)
			frame_switch_clientwin(frame, cwin);
		set_focus((WThing*)frame);
	}
#endif

	return TRUE;
}


WClientWin* manage_clientwin(Window win, int mflags)
{
	WClientWin *cwin;
	int state=NormalState;
	XWindowAttributes attr;
	XWMHints *hints;
	bool dock=FALSE;
	Window origwin=win;
	WWinProp *winprop;
	
again:
	/* catch UnmapNotify and DestroyNotify */
	XSelectInput(wglobal.dpy, win, StructureNotifyMask);
	
	if(!XGetWindowAttributes(wglobal.dpy, win, &attr)){
		warn("Window disappeared");
		goto fail2;
	}

	/* Get hints and check for dockapp */
	hints=XGetWMHints(wglobal.dpy, win);
	
	if(hints!=NULL && hints->flags&StateHint)
		state=hints->initial_state;

	if(!dock && state==WithdrawnState){
		if(hints->flags&IconWindowHint && hints->icon_window!=None){
			/* The dockapp might be displaying its "main" window if no
			 * wm that understands dockapps has been managing it.
			 */
			if(mflags&MANAGE_INITIAL)
				XUnmapWindow(wglobal.dpy, win);
	
			XSelectInput(wglobal.dpy, win, 0);
			
			win=hints->icon_window;
			
			/* Is the icon window already being managed? */
			cwin=find_clientwin(win);
			if(cwin!=NULL){
				if(WTHING_IS(cwin, WTHING_DOCKWIN))
					return cwin;
				unmanage_clientwin(cwin);
			}
			dock=TRUE;
			goto again;
		}
	}

	if(hints!=NULL)
		XFree((void*)hints);
	
	/* Get the actual state if any */
	get_win_state(win, &state);

	if(!dock && (attr.override_redirect ||
				 (mflags&MANAGE_INITIAL && attr.map_state!=IsViewable)))
		goto fail2;
	
	if(state!=NormalState && state!=IconicState)
		state=NormalState;

	/* Allocate and initialize */
	cwin=create_clientwin(win, 0, state, &attr);
	
	if(cwin==NULL)
		goto fail2;
	
	if(dock){
		winprop=find_winprop_win(origwin);	
		if(winprop!=NULL)
			cwin->dockpos=winprop->dockpos;
		
		if(!add_dockwin(cwin))
			goto failure;
	}else{
		if(!add_clientwin(cwin, &attr, mflags))
			goto failure;
	}
	
	/* Check that the window exists. The previous check does not seem
	 * to catch all cases of window destroyal.
	 */
	if(XGetWindowAttributes(wglobal.dpy, win, &attr))
		return cwin;
	
	warn("Window disappeared");

failure:
	unmanage_clientwin(cwin);
	return NULL;

fail2:
	XSelectInput(wglobal.dpy, win, 0);
	return NULL;
}


enum{
	DESTROY,
	UNMANAGE,
	UNMAP
};


static void do_unmanage_clientwin(WClientWin *cwin, int action)
{
	int x, y;
	
	destroy_subthings((WThing*)cwin);
	
	if(cwin->t_parent!=NULL){
		if(WTHING_IS(cwin->t_parent, WTHING_DOCK)){
			remove_dockwin(cwin);
		}else if(WTHING_IS(cwin->t_parent, WTHING_FRAME)){
			restore_gravity(CWIN_FRAME(cwin), cwin, &x, &y);
			frame_detach_clientwin(CWIN_FRAME(cwin), cwin, x, y);
		}
	}

	XSelectInput(wglobal.dpy, cwin->client_win, 0);
	
	if(action==UNMANAGE){
		/*set_win_state(cwin->client_win, NormalState);*/
		XMapWindow(wglobal.dpy, cwin->client_win);
	}

	if(action!=DESTROY)
		XRemoveFromSaveSet(wglobal.dpy, cwin->client_win);

	if(cwin->name!=NULL)
		XFree((void*)cwin->name);

	if(cwin->icon_name!=NULL)
		XFree((void*)cwin->icon_name);

	clientwin_unuse_label(cwin);
	if(cwin->label!=NULL)
		free(cwin->label);

	UNLINK_ITEM(SCREEN->clientwin_list, cwin, s_cwin_next, s_cwin_prev);
	SCREEN->n_clientwin--;

	XDeleteContext(wglobal.dpy, cwin->client_win, wglobal.win_context);

	if(cwin->orig_bw!=0)
		configure_cwin_bw(cwin->client_win, cwin->orig_bw);

	clear_colormaps(cwin);
	
	update_winlist();
	
	free_thing((WThing*)cwin);
}


/* Used when the the window is not to be managed anymore, but should
 * be mapped (deinit)
 */
void unmanage_clientwin(WClientWin *cwin)
{
	do_unmanage_clientwin(cwin, UNMANAGE);
}

/* Used when the window was unmapped */
void unmap_clientwin(WClientWin *cwin)
{
	do_unmanage_clientwin(cwin, UNMAP);
}


/* Used when the window was destroyed */
void destroy_clientwin(WClientWin *cwin)
{
	do_unmanage_clientwin(cwin, DESTROY);
}


/* */


void iconify_clientwin(WClientWin *cwin)
{
	D(warn("Ignoring request to iconify client window"));
}


void clientwin_detach(WClientWin *cwin)
{
	attachdetach_clientwin(NULL, cwin, FALSE, 0, 0);
}

/* */


void kill_clientwin(WClientWin *cwin)
{
	XKillClient(wglobal.dpy, cwin->client_win);
}


void send_clientmsg(Window win, Atom a)
{
	XClientMessageEvent ev;
	
	ev.type=ClientMessage;
	ev.window=win;
	ev.message_type=wglobal.atom_wm_protocols;
	ev.format=32;
	ev.data.l[0]=a;
	ev.data.l[1]=CurrentTime;
	
	XSendEvent(wglobal.dpy, win, False, 0L, (XEvent*)&ev);
}


void close_clientwin(WClientWin *cwin)
{
	if(cwin->flags&CWIN_P_WM_DELETE)
		send_clientmsg(cwin->client_win, wglobal.atom_wm_delete);
#if 0
	else
		kill_clientwin(cwin);
#endif
}


/* */


static void set_clientwin_state(WClientWin *cwin, int state)
{
	cwin->state=state;
	set_win_state(cwin->client_win, state);
}


void hide_clientwin(WClientWin *cwin)
{
	if(cwin==NULL)
		return;
	
	XSelectInput(wglobal.dpy, cwin->client_win,
				 cwin->event_mask&~(StructureNotifyMask|EnterWindowMask));
	XUnmapWindow(wglobal.dpy, cwin->client_win);
	XSelectInput(wglobal.dpy, cwin->client_win, cwin->event_mask);
	set_clientwin_state(cwin, IconicState);
}


void show_clientwin(WClientWin *cwin)
{
	if(cwin==NULL)
		return;
	
	XSelectInput(wglobal.dpy, cwin->client_win,
				 cwin->event_mask&~(StructureNotifyMask|EnterWindowMask));
	XMapRaised(wglobal.dpy, cwin->client_win);
	XSelectInput(wglobal.dpy, cwin->client_win, cwin->event_mask);
	set_clientwin_state(cwin, NormalState);
}


/* */


void set_clientwin_size(WClientWin *cwin, int w, int h)
{
	if(!CWIN_HAS_FRAME(cwin))
		return; /* Resizing of docked windows is not supported. */
		
	limit_size(cwin, &w, &h);
	frame_clientwin_resize(CWIN_FRAME(cwin), cwin, w, h, TRUE);
}


/* */


void clientwin_toggle_tagged(WClientWin *cwin)
{
	if(!CWIN_HAS_FRAME(cwin))
		return;
	
	cwin->flags^=CWIN_TAGGED;
	
	draw_frame_bar(CWIN_FRAME(cwin), TRUE);
}


/* */
	

bool clientwin_has_frame(WClientWin *cwin)
{
	WFrame *frame=CWIN_FRAME(cwin);
	
	return frame!=NULL && WTHING_IS(frame, WTHING_FRAME);
}


/* */


void clientwin_reconf_at(WClientWin *cwin, int rootx, int rooty)
{
	XEvent ce;
	Window win;
	
	if(cwin==NULL)
		return;
	
	win=cwin->client_win;
	
	ce.xconfigure.type=ConfigureNotify;
	ce.xconfigure.event=win;
	ce.xconfigure.window=win;
	ce.xconfigure.x=rootx;
	ce.xconfigure.y=rooty;
	ce.xconfigure.width=cwin->client_w;
	ce.xconfigure.height=cwin->client_h;
	ce.xconfigure.border_width=0;
	ce.xconfigure.above=None;
	ce.xconfigure.override_redirect=False;

	XSelectInput(wglobal.dpy, win, cwin->event_mask&~StructureNotifyMask);
	XSendEvent(wglobal.dpy, win, False, StructureNotifyMask, &ce);
	XSelectInput(wglobal.dpy, win, cwin->event_mask);
}


/* */


void attachdetach_clientwin(WFrame *frame, WClientWin *cwin,
							bool coords, int x, int y)
{
	WFrame *oldframe=NULL;
	bool is_new=FALSE;
	int w=0, h=0;
	
	if(CWIN_HAS_FRAME(cwin))
		oldframe=CWIN_FRAME(cwin);
		
	/* Same frame? */
	if(oldframe==frame)
		return;
	
	if(frame==NULL){
		/* No destination frame given */
		
		/* Only window - just bring it here? */
		if(oldframe!=NULL && oldframe->cwin_count==1){
			if(coords)
				set_frame_pos(oldframe, x, y);
			frame=oldframe;
			/* bring frame here */
			if(!on_current_workspace((WWinObj*)frame)){
				calc_placement(frame->w, frame->h, WORKSPACE_CURRENT, &x, &y);
				set_frame_pos(frame, x, y);
				move_to_workspace((WWinObj*)frame, WORKSPACE_CURRENT);
			}
			/*raise_winobj((WWinObj*)frame);*/
			return;
		}

		/* Detach - create new */

		if(!coords){
			clientwin_to_frame_size(cwin->client_w, cwin->client_h, 0, &w, &h);
			calc_placement(w, h, WORKSPACE_CURRENT, &x, &y);
		}
		
		frame=create_add_frame_simple(x, y, cwin->client_w, cwin->client_h);

		if(frame==NULL)
			return;
		
		frame_attach_clientwin(frame, cwin);
		set_frame_state(frame, 0);
	}else{
		frame_attach_clientwin(frame, cwin);
	}
}


/*{{{ Labels */


static char *untitled_label="<untitled>";


static const char *clientwin_beg_label(WClientWin *cwin)
{
	if(cwin->name!=NULL)
		return cwin->name;
	if(cwin->icon_name!=NULL)
		return cwin->icon_name;

	return untitled_label;
}


#define CLIENTNUM_TMPL "<%d>"


void clientwin_make_label(WClientWin *cwin, int maxw)
{
	const char *str=clientwin_beg_label(cwin);
	char tmp[16];

	if(cwin->label_inst!=0)
		sprintf(tmp, CLIENTNUM_TMPL, cwin->label_inst);
	else
		*tmp='\0';
		
	if(cwin->label!=NULL)
		free(cwin->label);
	
	cwin->label=make_label(GRDATA->font, str, tmp, maxw, &(cwin->label_width));
}


char *clientwin_full_label(WClientWin *cwin)
{
	const char *str=clientwin_beg_label(cwin);
	char tmp[16];
	
	if(cwin->label_inst!=0){
		sprintf(tmp, CLIENTNUM_TMPL, cwin->label_inst);
		return scat(str, tmp);
	}else{
		return scopy(str);
	}
}


#ifndef CF_NO_NUMBERING
static void use_label(WClientWin *cwin, const char *label)
{
	WClientWin *p, *np, *minp=NULL;
	int mininst=INT_MAX;
	const char *str;
	
	for(p=SCREEN->clientwin_list; p!=NULL; p=p->s_cwin_next){
		if(p==cwin)
			continue;
		str=clientwin_beg_label(p);
		if(strcmp(str, label)==0)
			break;
	}
	
	if(p==NULL)
		return;
	
	for(; ; p=np){
		assert(p!=cwin);
		np=p->label_next;
		if(p->label_inst+1==np->label_inst){
			continue;
		}else if(p->label_inst>=np->label_inst && np->label_inst!=0){
			mininst=0;
			minp=p;
		}else if(p->label_inst+1<mininst){
			mininst=p->label_inst+1;
			minp=p;
			continue;
		}
		break;
	}
	
	assert(minp!=NULL);
	
	np=minp->label_next;
	cwin->label_next=np;
	np->label_prev=cwin;
	minp->label_next=cwin;
	cwin->label_prev=minp;
	cwin->label_inst=mininst;
}
#endif

void clientwin_use_label(WClientWin *cwin)
{
	WFrame *frame=(WFrame*)cwin->t_parent;
#ifndef CF_NO_NUMBERING
	const char *p=clientwin_beg_label(cwin);
	
	use_label(cwin, p);
	
#endif
	if(frame!=NULL && WTHING_IS(frame, WTHING_FRAME))
		frame_recalc_bar(frame);
}


void clientwin_unuse_label(WClientWin *cwin)
{
#ifndef CF_NO_NUMBERING
	cwin->label_next->label_prev=cwin->label_prev;
	cwin->label_prev->label_next=cwin->label_next;
	cwin->label_next=cwin;
	cwin->label_prev=cwin;
	cwin->label_inst=0;
#endif
}
	
	
/*}}}*/
