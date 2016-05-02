# Generated automatically from Makefile.in by configure.
top_srcdir=		..
srcdir=			.
INSTALL=		/usr/bin/install -c
INSTALL_PROGRAM=	${INSTALL}

top_builddir= ..
include $(top_builddir)/Makefile.config

OBJS=		Endings.o EndingsDB.o Exact.o \
		Fuzzy.o Metaphone.o Soundex.o \
		SuffixEntry.o Synonym.o htfuzzy.o \
		Substring.o Prefix.o Accents.o

LIBOBJS=	Endings.o Exact.o Fuzzy.o Metaphone.o \
		Soundex.o Synonym.o EndingsDB.o SuffixEntry.o \
		Substring.o Prefix.o Accents.o

TARGET=		htfuzzy
LIBTARGET=	libfuzzy.a

all:	$(TARGET) $(LIBTARGET)

$(TARGET):	$(OBJS) $(HTLIBS)
	$(CXX) -o $(TARGET) $(LDFLAGS) $(OBJS) $(LIBS)

$(LIBTARGET):		$(LIBOBJS)
	$(AR) rcv $(LIBTARGET) $?
	$(RANLIB) $(LIBTARGET)

install:	$(TARGET)
	transform=s,x,x,
	$(INSTALL_PROGRAM) $(TARGET) $(INSTALL_ROOT)$(BIN_DIR)/`echo $(TARGET) | sed '$(transform)'`

clean:
	rm -f $(TARGET) $(LIBTARGET) $(OBJS) *~ *.bak *% a.out *.orig core

mostlyclean:
	rm -f $(TARGET) *~ *.bak *% a.out *.orig core

distclean:	clean
	rm -f Makefile *.proj

depend:
	$(MAKEDEPEND)  $(CXXFLAGS) $(INCS) -- $(OBJS)

htfuzzy.o:	$(top_builddir)/CONFIG
