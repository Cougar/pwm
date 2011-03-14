##
## Some make rules
##


# Main targets
######################################

ifdef MODULE
TARGETS := $(TARGETS) $(MODULE).la
endif

ifdef SUBDIRS

all: subdirs $(TARGETS)

clean: subdirs-clean _clean

realclean: subdirs-realclean _clean _realclean

depend: subdirs-depend _depend

else

all: $(TARGETS)

clean: _clean

realclean: _clean _realclean

depend: _depend

endif

ifdef INSTALL_SUBDIRS

install: subdirs-install _install

else

install: _install

endif


# The rules
######################################

ifdef MAKE_EXPORTS

TO_CLEAN := $(TO_CLEAN) exports.c

EXPORTS_C = exports.c

exports.c: $(SOURCES)
	$(LUA) $(TOPDIR)/mkexports.lua -module $(MAKE_EXPORTS) -o exports.c $(SOURCES)

else # !MAKE_EXPORTS

EXPORTS_C = 

endif # !MAKE_EXPORTS

ifdef MODULE

OBJS=$(subst .c,.lo,$(SOURCES) $(EXPORTS_C))

ifneq ($(PRELOAD_MODULES),1)
PICOPT=-prefer-pic
LINKOPT=-module -avoid-version
else
PICOPT=-static -prefer-non-pic
LINKOPT=-static -module -avoid-version
endif

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(PICOPT) $(CFLAGS) -c $< -o $@

$(MODULE).la: $(OBJS) $(EXT_OBJS)
	$(LIBTOOL) --mode=link $(CC) $(LINKOPT) $(LDFLAGS) \
 	-rpath $(MODULEDIR) $(OBJS) $(EXT_OBJS) -o $@

module_install:
	$(INSTALLDIR) $(MODULEDIR)
	$(LIBTOOL) --mode=install $(INSTALL) -m $(BIN_MODE) $(MODULE).la $(MODULEDIR)
	# $(STRIP) $(MODULEDIR)/$(MODULE).so

clean_objs:
	$(LIBTOOL) --mode=clean $(RM) -f $(OBJS)

clean_target:
	$(LIBTOOL) --mode=clean $(RM) -f $(MODULE).la

else #!MODULE

ifdef SOURCES

OBJS=$(subst .c,.o,$(SOURCES) $(EXPORTS_C))

clean_objs:
	$(RM) -f $(OBJS)

else #!SOURCES

clean_objs:

endif #!SOURCES

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean_target:
	rm -f $(TARGETS)

endif #!MODULE

_clean: clean_objs
	rm -f core $(DEPEND_FILE) $(TO_CLEAN)

_realclean: clean_target
	rm -f $(TO_REALCLEAN)


ifdef SOURCES

_depend:
	$(MAKE_DEPEND) $(SOURCES)
else #!SOURCES

_depend:
	
endif #!SOURCES


# Subdirectories
######################################

subdirs:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i; done

subdirs-depend:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i depend; done

subdirs-clean:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done

subdirs-realclean:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i realclean; done

subdirs-install:
	set -e; for i in $(INSTALL_SUBDIRS); do $(MAKE) -C $$i install; done


# Dependencies
######################################

ifeq ($(DEPEND_FILE),$(wildcard $(DEPEND_FILE)))
include $(DEPEND_FILE)
endif
