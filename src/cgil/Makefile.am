CGIL_FILES = ril.lm rlhc-main.lm \
	rlhc-c.lm rlhc-csharp.lm rlhc-go.lm rlhc-js.lm rlhc-ruby.lm \
	rlhc-crack.lm rlhc-d.lm rlhc-java.lm rlhc-julia.lm rlhc-ocaml.lm rlhc-rust.lm \
	rlhc-zig.lm

# The cgil sources translate ril, the intermediate code generation language
# emitted by libfsm, into the host languages. They are used only by ragel, so
# they go with it.
if INSTALL_RAGEL
pkgdata_DATA = $(CGIL_FILES)
endif

EXTRA_DIST = $(CGIL_FILES) CMakeLists.txt

# Each translator is also built into a standalone program. They belong to the
# ragel half of the suite, but are built either way so that test/cgil.d can run
# from the build tree.
CGIL_TRANSLATORS = \
	cgil-c cgil-crack cgil-csharp cgil-d \
	cgil-go cgil-java cgil-js cgil-julia \
	cgil-ocaml cgil-ruby cgil-rust cgil-zig

if INSTALL_RAGEL
bin_PROGRAMS = $(CGIL_TRANSLATORS)
else
noinst_PROGRAMS = $(CGIL_TRANSLATORS)
endif

# Each translator is made from its .lm files by the rules below, not from C
# that automake knows about. Declaring the sources empty keeps automake from
# assuming a cgil-<lang>.c and shipping the generated file in the tarball.
cgil_c_SOURCES =
cgil_crack_SOURCES =
cgil_csharp_SOURCES =
cgil_d_SOURCES =
cgil_go_SOURCES =
cgil_java_SOURCES =
cgil_js_SOURCES =
cgil_julia_SOURCES =
cgil_ocaml_SOURCES =
cgil_ruby_SOURCES =
cgil_rust_SOURCES =
cgil_zig_SOURCES =

CLEANFILES = cgil-*.c

cgil-c$(EXEEXT): $(srcdir)/rlhc-c.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-crack$(EXEEXT): $(srcdir)/rlhc-crack.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-csharp$(EXEEXT): $(srcdir)/rlhc-csharp.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-d$(EXEEXT): $(srcdir)/rlhc-d.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-go$(EXEEXT): $(srcdir)/rlhc-go.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-java$(EXEEXT): $(srcdir)/rlhc-java.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-js$(EXEEXT): $(srcdir)/rlhc-js.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-julia$(EXEEXT): $(srcdir)/rlhc-julia.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-ocaml$(EXEEXT): $(srcdir)/rlhc-ocaml.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-ruby$(EXEEXT): $(srcdir)/rlhc-ruby.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-rust$(EXEEXT): $(srcdir)/rlhc-rust.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<

cgil-zig$(EXEEXT): $(srcdir)/rlhc-zig.lm $(srcdir)/ril.lm $(srcdir)/rlhc-main.lm $(SUBJ_COLM_BIN)
	$(SUBJ_COLM_BIN) -B '$(top_builddir)' -I $(srcdir) -o $@ $<
