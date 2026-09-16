SUBDIRS = harness aapl.d colm.d ragel.d rlhc.d rlparse.d trans.d .

EXTRA_DIST = dist.sh

# Every suite runs through the one harness, after the suites have built what
# it needs. The harness takes its parallelism from MAKEFLAGS.
check-local:
	harness/harness

dist: dist.sh
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
