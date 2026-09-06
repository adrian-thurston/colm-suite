check_PROGRAMS = rlparse

rlparse_SOURCES = parse.c if.h if.cc commit.cc \
	actparams.cc parsetree.cc parsedata.cc \
	common.cc fsmap.cc fsmbase.cc idbase.cc \
	load.cc reducer.cc inputdata.cc fsmgraph.cc \
	main.cc dump.cc reducer.h vector.h inputdata.h \
	table.h gendata.h common.h ragel.h parsedata.h \
	action.h parsetree.h redfsm.h fsmgraph.h pcheck.h \
	load.h version.h config.h buffer.h \
	avlbasic.h avlcommon.h avlibasic.h avlikeyless.h \
	avlimap.h avlimel.h avlimelkey.h avliset.h avlitree.h \
	avlkeyless.h avlmap.h avlmel.h avlmelkey.h avlset.h \
	avltree.h compare.h bstcommon.h bstmap.h bstset.h \
	bsttable.h dlcommon.h dlist.h dlistmel.h dlistval.h \
	bubblesort.h mergesort.h sbstmap.h sbstset.h sbsttable.h \
	svector.h

rlparse_CPPFLAGS = $(LIBCOLM_CPPFLAGS)
rlparse_LDADD = $(LIBCOLM_LA)
rlparse_LDFLAGS = -static

EXTRA_DIST = \
	reducer.lm rlparse.lm ragel.lm \
	ragel-c.lm ragel-crack.lm ragel-ocaml.lm \
	ragel-ruby.lm \
	case/rlparse.h case/rlscan.rl \
	case/rlscan--colm-frontend.exp case/rlscan--reduce-frontend.exp

parse.c: rlparse.lm reducer.lm $(RAGEL_LM) $(COLM)
	$(COLM) -c -b rlparse_object -o $@ -e if.h -x if.cc -m commit.cc -I$(srcdir) $<

if.h: parse.c
if.cc: parse.c
commit.cc: parse.c

load.cc: if.h
reducer.cc: if.h

CLEANFILES = parse.c if.h if.cc commit.cc

clean-local:
	rm -Rf working
