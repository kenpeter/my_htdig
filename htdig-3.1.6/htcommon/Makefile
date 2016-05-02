# Generated automatically from Makefile.in by configure.
top_srcdir=		..
srcdir=			.
INSTALL=		/usr/bin/install -c
INSTALL_PROGRAM=	${INSTALL}

top_builddir= ..
include $(top_builddir)/Makefile.config

OBJS=	DocumentDB.o DocumentRef.o WordList.o \
		defaults.o 

TARGET=		libcommon.a
LOCAL_DEFINES=	-DBIN_DIR=\"$(BIN_DIR)\" \
		-DCOMMON_DIR=\"$(COMMON_DIR)\" \
		-DCONFIG_DIR=\"$(CONFIG_DIR)\" \
		-DDATABASE_DIR=\"$(DATABASE_DIR)\" \
		-DIMAGE_URL_PREFIX=\"$(IMAGE_URL_PREFIX)\" \
		-DPDF_PARSER=\"$(PDF_PARSER)\" \
		-DSORT_PROG=\"/bin/sort\"

all:		$(TARGET)

$(TARGET):	$(OBJS)
	$(AR) rcv $(TARGET) $?
	$(RANLIB) $(TARGET)

install:	$(TARGET)
	@echo "Nothing to install"

clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak *% a.out core *.orig

mostlyclean:
	rm -f *~ *.bak *% a.out core *.orig

distclean:	clean
	rm -f Makefile *.proj

depend:
	$(MAKEDEPEND) $(CXXFLAGS)  $(INCS) -- $(OBJS)

defaults.o:		$(top_builddir)/CONFIG
