# The cgil translators belong to the ragel half of the suite. They are built
# either way so the test suite here can run from the build tree.
if INSTALL_RAGEL
bin_PROGRAMS = \
	cgil-c cgil-crack cgil-csharp cgil-d \
	cgil-go cgil-java cgil-js cgil-julia \
	cgil-ocaml cgil-ruby cgil-rust cgil-zig
else
noinst_PROGRAMS = \
	cgil-c cgil-crack cgil-csharp cgil-d \
	cgil-go cgil-java cgil-js cgil-julia \
	cgil-ocaml cgil-ruby cgil-rust cgil-zig
endif

check_SCRIPTS = runtests

pkgdata_SCRIPTS = runtests

TESTS = runtests

EXTRA_DIST = runtests.in cases

runtests: runtests.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

CLEANFILES = runtests cgil-*.c

cgil-c$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-c.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN) 
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-crack$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-crack.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN) 
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-csharp$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-csharp.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-d$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-d.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-go$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-go.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-java$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-java.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-js$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-js.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-julia$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-julia.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-ocaml$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-ocaml.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-ruby$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-ruby.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-rust$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-rust.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<

cgil-zig$(EXEEXT): $(top_srcdir)/src/cgil/rlhc-zig.lm $(top_srcdir)/src/cgil/ril.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(top_srcdir)/src/cgil -o $@ $<
