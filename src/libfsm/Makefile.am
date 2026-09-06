# libfsm contains only the FSM construction code and the backend code
# generators for C, asm and cgil (Code Gen Intermediate Language) . It is
# useful for building state machine code generators in programs not connected
# to the ragel language.
#
# libfsm is part of the ragel half of the suite. When it is not being
# installed it is still built, as a convenience library, so the ragel
# programs can be built and tested from the build tree.
#
# The library is assembled from two convenience libraries: libfsmgraph, the
# state machine construction library, and libfsmgen, the reduction and code
# generation half. Colm builds its token scanners on libfsmgraph and links it
# statically, so the colm binary never depends on an installed libfsm.
#
noinst_LTLIBRARIES = libfsmgraph.la libfsmgen.la

if INSTALL_RAGEL
lib_LTLIBRARIES = libfsm.la

libfsminclude_HEADERS = \
	action.h fsmgraph.h common.h \
	gendata.h redfsm.h dot.h asm.h ragel.h \
	binary.h bingoto.h binbreak.h binvar.h \
	flat.h flatgoto.h flatbreak.h flatvar.h \
	switch.h switchgoto.h switchbreak.h switchvar.h \
	goto.h gotoloop.h gotoexp.h \
	parsedata.h idbase.h codegen.h \
	tables.h \
	actloop.h actexp.h \
	ipgoto.h
else
noinst_LTLIBRARIES += libfsm.la
endif

#
# libfsm takes nothing from configure: no config.h, no HAVE_* or SIZEOF_*
# macros. nostdinc drops automake's implicit -I$(top_builddir)/src/colm, so a
# stray include of colm's config.h fails to compile rather than quietly
# succeeding.
#
AUTOMAKE_OPTIONS = nostdinc

#
# libfsmgraph: state machine construction.
#
libfsmgraph_la_CPPFLAGS = -I$(top_srcdir)/src/aapl

dist_libfsmgraph_la_SOURCES = \
	idbase.cc fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc fsmgraph.cc \
	fsmap.cc fsmcond.cc fsmnfa.cc

#
# libfsmgen: reduction and direct code generation.
#
libfsmgen_la_CPPFLAGS = -I$(top_srcdir)/src/aapl

dist_libfsmgen_la_SOURCES = \
	redfsm.cc gendata.cc \
	codegen.cc \
	actexp.cc binvar.cc \
	tables.cc tabgoto.cc tabbreak.cc tabvar.cc \
	binary.cc bingoto.cc binbreak.cc actloop.cc \
	flat.cc flatgoto.cc flatbreak.cc flatvar.cc \
	switch.cc switchgoto.cc switchbreak.cc switchvar.cc \
	goto.cc gotoloop.cc gotoexp.cc ipgoto.cc \
	dot.cc asm.cc

#
# libfsm: both halves. The source list is empty; the EXTRA C++ source makes
# automake link with the C++ driver.
#
libfsm_la_SOURCES =
nodist_EXTRA_libfsm_la_SOURCES = dummy.cc
libfsm_la_LIBADD = libfsmgraph.la libfsmgen.la

if INSTALL_RAGEL
libfsm_la_LDFLAGS = -release ${LIBFSM_VERSION} -no-undefined

if LINKER_NO_UNDEFINED
libfsm_la_LDFLAGS += -Wl,--no-undefined
endif
endif

EXTRA_DIST = CMakeLists.txt
