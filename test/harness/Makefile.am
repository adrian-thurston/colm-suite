# The test harness: enumerates every suite under test/ into one work queue
# and runs it. It is independent of the colm and ragel under test.

noinst_PROGRAMS = harness

harness_SOURCES = \
	main.cc run.cc process.cc casefile.cc diff.cc util.cc \
	aapl.cc colm.cc ragel.cc rlhc.cc rlparse.cc trans.cc \
	harness.h

nodist_harness_SOURCES = config.h

harness_CPPFLAGS = -I. -I$(top_srcdir)/src/aapl
harness_CXXFLAGS = -std=c++11 -pthread
harness_LDFLAGS = -pthread

BUILT_SOURCES = config.h
EXTRA_DIST = config.h.in
CLEANFILES = config.h

# The subject paths and toolchains configure found, plus the tree layout.
config.h: config.h.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w $(SED_SUBST) \
		-e 's|[@]abs_top_srcdir[@]|$(abs_top_srcdir)|g' \
		-e 's|[@]abs_top_builddir[@]|$(abs_top_builddir)|g'
