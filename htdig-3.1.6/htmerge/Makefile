# Generated automatically from Makefile.in by configure.
top_srcdir=		..
srcdir=			.
INSTALL=		/usr/bin/install -c
INSTALL_PROGRAM=	${INSTALL}

top_builddir= ..
include $(top_builddir)/Makefile.config

OBJS=		db.o docs.o htmerge.o words.o 

TARGET=		htmerge
LOCAL_DEFINES=	-DSORT_PROG=\"/bin/sort\"

all:		$(TARGET)

$(TARGET):	$(OBJS) $(HTLIBS)
	$(CXX) -o $(TARGET) $(LDFLAGS) $(OBJS) $(LIBS)

install:	$(TARGET)
	transform=s,x,x,
	$(INSTALL_PROGRAM) $(TARGET) $(INSTALL_ROOT)$(BIN_DIR)/`echo $(TARGET) | sed '$(transform)'`

clean:
	rm -f $(OBJS) $(TARGET) *~ *% *.bak core a.out *.orig

mostlyclean:	clean

distclean:	clean
	rm -f Makefile *.proj

depend:
	$(MAKEDEPEND) $(CXXFLAGS) $(INCS) -- $(OBJS)

htmerge.o:		$(top_builddir)/CONFIG
