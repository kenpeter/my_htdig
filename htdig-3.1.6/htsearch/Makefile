# Generated automatically from Makefile.in by configure.
top_srcdir=		..
srcdir=			.
INSTALL=		/usr/bin/install -c
INSTALL_PROGRAM=	${INSTALL}

top_builddir= ..
include $(top_builddir)/Makefile.config

OBJS=		Display.o DocMatch.o ResultList.o ResultMatch.o \
		Template.o TemplateList.o WeightWord.o htsearch.o \
		parser.o

FOBJS=		$(top_builddir)/htfuzzy/libfuzzy.a
TARGET=		htsearch
LOCAL_DEFINES=	-DCONFIG_DIR=\"$(CONFIG_DIR)\"

all:	$(TARGET)

$(TARGET):	$(OBJS) $(FOBJS) $(HTLIBS)
	$(CXX) -o $(TARGET) $(LDFLAGS) $(OBJS) $(FOBJS) $(LIBS)

install:	all
	transform=s,x,x,
	$(INSTALL_PROGRAM) $(TARGET) $(INSTALL_ROOT)$(CGIBIN_DIR)/`echo $(TARGET) | sed '$(transform)'`

clean:
	rm -f $(OBJS) $(TARGET) *~ *.bak *% core *.orig a.out

mostlyclean:	clean

distclean:	clean
	rm -f Makefile *.proj

depend:
	$(MAKEDEPEND)  $(CXXFLAGS) $(INCS) -- $(OBJS)

htsearch.o:		$(top_builddir)/CONFIG
