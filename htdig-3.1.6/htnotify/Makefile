# Generated automatically from Makefile.in by configure.
top_srcdir=		..
srcdir=			.
INSTALL=		/usr/bin/install -c
INSTALL_PROGRAM=	${INSTALL}

top_builddir= ..
include $(top_builddir)/Makefile.config

OBJS=		EmailNotification.o htnotify.o

LOCAL_DEFINES=  -DSENDMAIL=\"$(SENDMAIL)\"

TARGET=		htnotify

all:	$(TARGET)

$(TARGET):	$(OBJS) $(HTLIBS)
	$(CXX) -o $(TARGET) $(LDFLAGS) $(OBJS) $(LIBS)

install:	$(TARGET)
	transform=s,x,x,
	$(INSTALL_PROGRAM) $(TARGET) $(INSTALL_ROOT)$(BIN_DIR)/`echo $(TARGET) | sed '$(transform)'`

clean:
	rm -f $(TARGET) $(OBJS) *~ *.bak *% a.out *.orig core

mostlyclean:	clean

distclean:	clean
	rm -f Makefile *.proj

depend:
	$(MAKEDEPEND)  $(CXXFLAGS) $(INCS) -- $(OBJS)

htnotify.o:		$(top_builddir)/CONFIG
