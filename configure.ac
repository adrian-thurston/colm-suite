dnl 
dnl Copyright 2006-2020 Adrian Thurston <thurston@colm.net>
dnl

dnl Permission is hereby granted, free of charge, to any person obtaining a copy
dnl of this software and associated documentation files (the "Software"), to
dnl deal in the Software without restriction, including without limitation the
dnl rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
dnl sell copies of the Software, and to permit persons to whom the Software is
dnl furnished to do so, subject to the following conditions:
dnl 
dnl The above copyright notice and this permission notice shall be included in all
dnl copies or substantial portions of the Software.
dnl 
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
dnl AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
dnl OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
dnl SOFTWARE.
  
dnl The suite is a calendar-versioned distribution: YYYY.MM, zero padded,
dnl with an optional micro for fixups (2026.08, then 2026.08.1) and -pre.N
dnl for pre-releases. Each component carries its own version, declared below.
AC_INIT([colm-suite],[2026.08])
PUBDATE="August 2026"

dnl
dnl Component versions. Every shipped component is versioned independently
dnl of the suite: colm and ragel continue their own lines, aapl resumes the
dnl line that ended at 2.14 in 2006, libfsm and cgil start their own.
dnl
COLM_VERSION="0.15.0"
COLM_PUBDATE="August 2026"

AAPL_VERSION="2.15.0"
LIBFSM_VERSION="1.0.0"
CGIL_VERSION="1.0.0"

AM_INIT_AUTOMAKE([foreign])

: ${CFLAGS="-Wall -g"}
: ${CXXFLAGS="-Wall -g"}

AC_CONFIG_MACRO_DIR([m4])

AC_SUBST(PUBDATE)
AC_DEFINE_UNQUOTED([PUBDATE], ["$PUBDATE"], [publication date])

AC_SUBST(COLM_VERSION)
AC_SUBST(COLM_PUBDATE)
AC_SUBST(AAPL_VERSION)
AC_SUBST(LIBFSM_VERSION)
AC_SUBST(CGIL_VERSION)
AC_DEFINE_UNQUOTED([COLM_VERSION], ["$COLM_VERSION"], [colm version])
AC_DEFINE_UNQUOTED([COLM_PUBDATE], ["$COLM_PUBDATE"], [colm publication date])

AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([void *])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([unsigned long])
AC_CHECK_SIZEOF([unsigned long long])
AC_CHECK_HEADERS([sys/mman.h sys/wait.h unistd.h])
AC_SYS_LARGEFILE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_CHECK_TOOL(AR, ar)
AC_PROG_RANLIB
LT_INIT
AC_PROG_LN_S

SED_SUBST=["\
	-e 's|@CXX@|${CXX}|g' \
	-e 's|@CC@|${CC}|g' \
	-e 's|@prefix@|${prefix}|g' \
"]

dnl Set test on c++ compiler.
AC_LANG([C++])

dnl
dnl libfsmincludedir
dnl
libfsmincludedir='${includedir}/libfsm/'
AC_SUBST(libfsmincludedir)
SED_SUBST="$SED_SUBST -e 's|[@]libfsmincludedir[@]|$libfsmincludedir|g'"

dnl
dnl aaplincludedir
dnl
aaplincludedir='${includedir}/aapl/'
AC_SUBST(aaplincludedir)
SED_SUBST="$SED_SUBST -e 's|[@]aaplincludedir[@]|$aaplincludedir|g'"

dnl Check for definition of MAKE.
AC_PROG_MAKE_SET

AC_ARG_ENABLE(pool-malloc, 
		AS_HELP_STRING([--enable-pool-malloc],[allocate pool objects with malloc]), 
		AC_DEFINE([POOL_MALLOC], [1], [allocate pool objects with malloc]))

AC_ARG_ENABLE(debug,
		AS_HELP_STRING([--enable-debug],[enable debug statements]), 
		AC_DEFINE([DEBUG], [1], [enable debug statements]))


dnl Do not build the manual by default. Most of the time the dependencies are
dnl not available, as they can be quite big installs.
build_manual=no

dnl
dnl Enable arg to explicitly control the building of the manual
dnl
AC_ARG_ENABLE(manual,
	[  --enable-manual         do we want to build the manual?],
	[
		if test "x$enableval" = "xyes"; then
			build_manual=yes;

			AC_CHECK_PROG([ASCIIDOC], [asciidoc], [asciidoc])
			AC_CHECK_PROG([PYGMENTIZE], [pygmentize], [pygmentize])

			if [test "x$ASCIIDOC" == x || test "x$PYGMENTIZE" == x]; then
				AC_MSG_ERROR(must have both asciidoc and pygmentize to build the manual)
			fi 
		fi
	],
)

dnl Checks to carry out if we are building the manual.
dnl if test "x$build_manual" = "xyes"; then
dnl	AC_CHECK_PROG(FIG2DEV, fig2dev, fig2dev)
dnl	if test -z "$FIG2DEV"; then
dnl		echo
dnl		echo "error: fig2dev is required to build the manual (maybe use --disable-manual)"
dnl		echo
dnl		exit 1
dnl	fi
dnl
dnl	AC_CHECK_PROG(PDFLATEX, pdflatex, pdflatex)
dnl	if test -z "$PDFLATEX"; then
dnl		echo
dnl		echo "error: pdflatex is required to build the manual (maybe use --disable-manual)"
dnl		echo
dnl		exit 1
dnl	fi
dnl fi

dnl Set to true if the manual should be built.
AM_CONDITIONAL(BUILD_MANUAL, [test "x$build_manual" = "xyes"])

dnl
dnl Build the program? ( no means just build libfsm )
dnl
AC_ARG_ENABLE(program,
	[AS_HELP_STRING([--enable-program],[build the program? no means libfsm only ])],
	[
		if test "x$enableval" = "xyes"; then
			build_program=yes;
		else
			build_program=no;
		fi
	],
	[
		build_program=yes;
	]
)

AM_CONDITIONAL([BUILD_PROGRAM],    [test "x$build_program" = "xyes"])
AM_CONDITIONAL([WITH_RAGEL_KELBT], [test "x$RAGEL_KELBT" = "xyes"])

dnl
dnl Which halves of the suite to install. The build always covers the whole
dnl tree: ragel's parsers are written in colm, so colm must be built either
dnl way. These options control only what "make install" copies out.
dnl
dnl The colm runtime library and the aapl headers are installed in both cases.
dnl Ragel links libcolm and the installed libfsm headers include aapl, so
dnl neither can be left out of a ragel-only install.
dnl
AC_ARG_ENABLE(install-colm,
	[AS_HELP_STRING([--disable-install-colm],[do not install the colm program and headers])],
	[install_colm=$enableval],
	[install_colm=yes]
)

AC_ARG_ENABLE(install-ragel,
	[AS_HELP_STRING([--disable-install-ragel],[do not install ragel, libragel, libfsm or cgil])],
	[install_ragel=$enableval],
	[install_ragel=yes]
)

if test "x$install_colm" != "xyes" && test "x$install_ragel" != "xyes"; then
	AC_MSG_ERROR([nothing left to install: give at most one of --disable-install-colm and --disable-install-ragel])
fi

AM_CONDITIONAL([INSTALL_COLM],  [test "x$install_colm"  = "xyes"])
AM_CONDITIONAL([INSTALL_RAGEL], [test "x$install_ragel" = "xyes"])

dnl This is from ragel, but already have one above from colm. Need to merge.
dnl AM_CONDITIONAL([BUILD_MANUAL],     [test "x$build_manual" = "xyes"])

AC_CANONICAL_HOST()
AM_CONDITIONAL([LINKER_NO_UNDEFINED], [test "x$host_os" = "xlinux-gnu"])

dnl
dnl Testing
dnl
dnl Which installed target to test. Without this option supplied testing is
dnl done against what is in the source tree.
dnl
AC_ARG_WITH(subject,
	[AS_HELP_STRING([--with-subject],[location of install tree test (defaults source tree)])],
	[
		SUBJ_AAPL_CPPFLAGS="-I${withval}/include/aapl"

		SUBJ_COLM_BIN="${withval}/bin/colm"
		SUBJ_COLM_CPPFLAGS="-I${withval}/include"
		SUBJ_COLM_LDFLAGS="-L${withval}/lib -Wl,-rpath,${withval}/lib"

		SUBJ_RAGEL_BIN="$withval/bin/ragel"
		SUBJ_RAGEL_CPPFLAGS="-I$withval/include"
		SUBJ_RAGEL_LDFLAGS="-L$withval/lib -Wl,-rpath,${withval}/lib"
		SUBJ_RAGEL_LM="${withval}/share"

		SUBJ_RAGEL_C_BIN="$withval/bin/ragel-c"
		SUBJ_RAGEL_D_BIN="$withval/bin/ragel-d"
		SUBJ_RAGEL_JAVA_BIN="$withval/bin/ragel-java"
		SUBJ_RAGEL_RUBY_BIN="$withval/bin/ragel-ruby"
		SUBJ_RAGEL_CSHARP_BIN="$withval/bin/ragel-csharp"
		SUBJ_RAGEL_GO_BIN="$withval/bin/ragel-go"
		SUBJ_RAGEL_OCAML_BIN="$withval/bin/ragel-ocaml"
		SUBJ_RAGEL_ASM_BIN="$withval/bin/ragel-asm"
		SUBJ_RAGEL_RUST_BIN="$withval/bin/ragel-rust"
		SUBJ_RAGEL_ZIG_BIN="$withval/bin/ragel-zig"
		SUBJ_RAGEL_CRACK_BIN="$withval/bin/ragel-crack"
		SUBJ_RAGEL_JULIA_BIN="$withval/bin/ragel-julia"
	],
	[
		SUBJ_AAPL_CPPFLAGS='-I$(abs_top_srcdir)/src/aapl'

		SUBJ_COLM_BIN='$(abs_top_builddir)/src/colm/colm'
		SUBJ_COLM_CPPFLAGS='-I$(abs_top_builddir)/src/colm/include'
		SUBJ_COLM_LDFLAGS='-L$(abs_top_builddir)/src/colm/.libs -Wl,-rpath,${abs_top_builddir}/src/colm/.libs'

		SUBJ_RAGEL_BIN='$(abs_top_builddir)/src/ragel/ragel'
		SUBJ_RAGEL_CPPFLAGS='-I$(abs_top_builddir)/src/ragel/include -I$(abs_top_builddir)/src/aapl'
		SUBJ_RAGEL_LDFLAGS='-L$(abs_top_builddir)/src/ragel/.libs -Wl,-rpath,$(abs_top_builddir)/src/ragel/.libs'
		SUBJ_RAGEL_LM='$(abs_top_builddir)/src/ragel'

		SUBJ_RAGEL_C_BIN='$(abs_top_builddir)/src/ragel/host-c/ragel-c'
		SUBJ_RAGEL_D_BIN='$(abs_top_builddir)/src/ragel/host-d/ragel-d'
		SUBJ_RAGEL_JAVA_BIN='$(abs_top_builddir)/src/ragel/host-java/ragel-java'
		SUBJ_RAGEL_RUBY_BIN='$(abs_top_builddir)/src/ragel/host-ruby/ragel-ruby'
		SUBJ_RAGEL_CSHARP_BIN='$(abs_top_builddir)/src/ragel/host-csharp/ragel-csharp'
		SUBJ_RAGEL_GO_BIN='$(abs_top_builddir)/src/ragel/host-go/ragel-go'
		SUBJ_RAGEL_OCAML_BIN='$(abs_top_builddir)/src/ragel/host-ocaml/ragel-ocaml'
		SUBJ_RAGEL_ASM_BIN='$(abs_top_builddir)/src/ragel/host-asm/ragel-asm'
		SUBJ_RAGEL_RUST_BIN='$(abs_top_builddir)/src/ragel/host-rust/ragel-rust'
		SUBJ_RAGEL_ZIG_BIN='$(abs_top_builddir)/src/ragel/host-zig/ragel-zig'
		SUBJ_RAGEL_CRACK_BIN='$(abs_top_builddir)/src/ragel/host-crack/ragel-crack'
		SUBJ_RAGEL_JULIA_BIN='$(abs_top_builddir)/src/ragel/host-julia/ragel-julia'
	]
)

SED_SUBST="$SED_SUBST -e 's|@SUBJ_COLM_BIN@|${SUBJ_COLM_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_COLM_CPPFLAGS@|${SUBJ_COLM_CPPFLAGS}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_COLM_LDFLAGS@|${SUBJ_COLM_LDFLAGS}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_BIN@|${SUBJ_RAGEL_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_CPPFLAGS@|${SUBJ_RAGEL_CPPFLAGS}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_LDFLAGS@|${SUBJ_RAGEL_LDFLAGS}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_LM@|${SUBJ_RAGEL_LM}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_C_BIN@|${SUBJ_RAGEL_C_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_D_BIN@|${SUBJ_RAGEL_D_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_JAVA_BIN@|${SUBJ_RAGEL_JAVA_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_RUBY_BIN@|${SUBJ_RAGEL_RUBY_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_CSHARP_BIN@|${SUBJ_RAGEL_CSHARP_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_GO_BIN@|${SUBJ_RAGEL_GO_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_OCAML_BIN@|${SUBJ_RAGEL_OCAML_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_ASM_BIN@|${SUBJ_RAGEL_ASM_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_RUST_BIN@|${SUBJ_RAGEL_RUST_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_ZIG_BIN@|${SUBJ_RAGEL_ZIG_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_CRACK_BIN@|${SUBJ_RAGEL_CRACK_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@SUBJ_RAGEL_JULIA_BIN@|${SUBJ_RAGEL_JULIA_BIN}|g'"

AC_SUBST(SUBJ_AAPL_CPPFLAGS)

AC_SUBST(SUBJ_COLM_BIN)
AC_SUBST(SUBJ_COLM_CPPFLAGS)
AC_SUBST(SUBJ_COLM_LDFLAGS)

AC_SUBST(SUBJ_RAGEL_BIN)
AC_SUBST(SUBJ_RAGEL_CPPFLAGS)
AC_SUBST(SUBJ_RAGEL_LDFLAGS)
AC_SUBST(SUBJ_RAGEL_LM)

dnl
dnl Installed programs to test in 
dnl

AC_PATH_PROG([D_BIN], [gdc-8])
if test "x$D_BIN" = x; then
	AC_PATH_PROG([D_BIN], [gdc-7])
	if test "x$D_BIN" = x; then
		AC_PATH_PROG([D_BIN], [gdc-6])
		if test "x$D_BIN" = x; then
			AC_PATH_PROG([D_BIN], [gdc-5])
			if test "x$D_BIN" = x; then
				dnl Newer distributions ship gdc-11 and later, which this chain
				dnl does not know about, alongside an unversioned gdc.
				AC_PATH_PROG([D_BIN], [gdc])
			fi
		fi
	fi
fi

AC_PATH_PROG([JAVAC_BIN], [javac])

dnl
dnl Java test cases are now requring a large virtual address space. On systems
dnl where this is limited by ulimit, java will fail to run.
dnl

if test "x$JAVAC_BIN" != x; then
	echo 'class conftest { public static void main( String[] args ) { } }' > conftest.java
	AC_MSG_CHECKING([checking if javac is able to compile programs ... ])
	if sh -c "$JAVAC_BIN conftest.java" >>config.log 2>&1; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		JAVAC_BIN=""
	fi
	rm -f conftest.java conftest.class
fi

AC_PATH_PROG([RUBY_BIN], [ruby])
AC_PATH_PROG([CSHARP_BIN], [mcs])
AC_PATH_PROG([GO_BIN], [go])
AC_PATH_PROG([OCAML_BIN], [ocaml])
AC_PATH_PROG([RUST_BIN], [rustc])
AC_PATH_PROG([JULIA_BIN], [julia])
AC_PATH_PROG([ZIG_BIN], [zig])

dnl
dnl Julia requires a large virtual address space. On systems where this is
dnl limited by ulimit, julia will fail to run.
dnl

if test "x$JULIA_BIN" != x; then
	echo 'println( "can run julia programs" );' > conftest.jl
	AC_MSG_CHECKING([checking if julia is able to run programs ... ])
	if sh -c "$JULIA_BIN conftest.jl" >>config.log 2>&1; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		JULIA_BIN=""
	fi
	rm -f conftest.jl
fi

dnl We can run julia, now make sure we have 1.0
if test "x$JULIA_BIN" != x; then
	AC_MSG_CHECKING([checking if julia is version 1.0 or later ... ])

	dnl We assume the form "julia version X.X.X"
	dnl   1. everything before the version number.
	dnl   2. Remove trailing version.
	dnl   3. Verify it is plain number greater than 1.
	dnl Any failure comes out as empty string. Note the sed command is wrapped
	dnl in [] so autotools do not interpret anything in it.
	JULIA1="`$JULIA_BIN -v | sed ['s/[A-Za-z ]\+//g; s/\.[0-9\.]*//; /^[1-9][0-9]*/!d;']`"
	if test "x$JULIA1" != x; then
		AC_MSG_RESULT([yes]) 
	else
		AC_MSG_RESULT([no]) 
		JULIA_BIN=""
	fi
fi

AC_PATH_PROG([GNUSTEP_CONFIG], [gnustep-config])

dnl
dnl The ASM tests have been based off of sources that are non-relocatable
dnl These won't assemble on some systems (with the right architecture). Check
dnl our specific flavour of ASM now so we can avoid some test failures.
dnl

ASM_BIN=$CC

cat <<\EOF > conftest.s
.section .rodata
.L_works:
.string "works"

.text
.globl	main
main:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rdi
	movq	$.L_works, %rdi
	call	puts
	popq	%rdi
	mov	$0, %rax
	ret
EOF

AC_MSG_CHECKING([checking if ragel ASM tests will build ... ])
if sh -c "$ASM_BIN -no-pie -o conftest.bin conftest.s" >>config.log 2>&1; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	ASM_BIN=""
fi
rm -f conftest.s conftest.bin

SED_SUBST="$SED_SUBST -e 's|@D_BIN@|${D_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@JAVAC_BIN@|${JAVAC_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@RUBY_BIN@|${RUBY_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@CSHARP_BIN@|${CSHARP_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@GO_BIN@|${GO_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@OCAML_BIN@|${OCAML_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@RUST_BIN@|${RUST_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@JULIA_BIN@|${JULIA_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@ZIG_BIN@|${ZIG_BIN}|g'"
SED_SUBST="$SED_SUBST -e 's|@GNUSTEP_CONFIG@|${GNUSTEP_CONFIG}|g'"
SED_SUBST="$SED_SUBST -e 's|@ASM_BIN@|${ASM_BIN}|g'"

AC_ARG_WITH(crack,
	[AS_HELP_STRING([--with-crack],[location of crack install])],
	[CRACK_BIN="$withval/bin/crack"],
	[AC_PATH_PROG([CRACK_BIN], [crack])]
)
SED_SUBST="$SED_SUBST -e 's|@CRACK_BIN@|${CRACK_BIN}|g'"

AC_SUBST(CRACK_BIN)

dnl Skip bootstrap process and build using an existing version of colm. Allows us to
dnl break colm without breaking the build of colm.
dnl 
dnl WARNING: DO NOT USE FOR PRODUCTION BUILD
dnl
dnl Only use this when you need to in order to get colm to build. Using an external
dnl colm program (and library) has the consequence that parsing of patterns and
dnl constructors in colm programs will also use this external library, since there
dnl can only be one set of colm functions linked. It's not possible to use the
dnl external parser for parsing the colm program, then use the internal parser for
dnl the patterns.
AC_ARG_WITH(colm,
	[AS_HELP_STRING([--with-colm],[prefix of colm install to build with, instead of bootstrapping])],
	[
		EXTERNAL_COLM="$withval"
		EXTERNAL_INC="-I$withval/include"
		EXTERNAL_LIBS="-L$withval/lib"
		if test "$cross_compiling" != "yes"; then
		AC_CHECK_FILES(["$EXTERNAL_COLM/bin/colm"], [],
			[AC_MSG_ERROR("could not find $EXTERNAL_COLM/bin/colm")])
		else
		AS_IF([test -a "$EXTERNAL_COLM/bin/colm"], [],
			AC_MSG_ERROR("could not find $EXTERNAL_COLM/bin/colm"))
		fi
	],
	[]
)

AM_CONDITIONAL([EXTERNAL_COLM], [test "x$EXTERNAL_COLM" != x])
AC_SUBST(EXTERNAL_COLM)
AC_SUBST(EXTERNAL_INC)
AC_SUBST(EXTERNAL_LIBS)

dnl Check for fopencookie. If available, we will use to avoid leaking FILE structs.
dnl The result of an fdopen cannot be closed without also closing the fd, so we
dnl make our own FILE type.
AC_CHECK_FUNC(fopencookie,
	[AC_DEFINE([HAVE_FOPENCOOKIE], [1], [have fopencookie])],
	[]
)


dnl
dnl Wrap up.
dnl

AC_SUBST(SED_SUBST)

dnl
dnl Ragel build configuration
dnl
RAGEL_VERSION="7.1.0"
RAGEL_PUBDATE="August 2026"
AC_SUBST(RAGEL_VERSION)
AC_SUBST(RAGEL_PUBDATE)

dnl The ragel man page is generated with sedsubst, not config.status, so it
dnl needs these in SED_SUBST to be filled in.
SED_SUBST="$SED_SUBST -e 's|[@]RAGEL_VERSION[@]|$RAGEL_VERSION|g'"
SED_SUBST="$SED_SUBST -e 's|[@]RAGEL_PUBDATE[@]|$RAGEL_PUBDATE|g'"

dnl Use local colm components
COLM='$(abs_top_builddir)/src/colm/colm'
COLM_WRAP='$(abs_top_builddir)/src/colm/colm-wrap'
COLM_SHARE='$(abs_top_srcdir)/src/cgil'
LIBCOLM_LA='$(abs_top_builddir)/src/colm/libcolm.la'
LIBCOLM_CPPFLAGS='-I$(abs_top_builddir)/src/colm/include'
LIBFSM_LA='$(abs_top_builddir)/src/libfsm/libfsm.la'
LIBFSM_CPPFLAGS='-I$(abs_top_srcdir)/src'

AC_SUBST(COLM)
AC_SUBST(COLM_WRAP)
AC_SUBST(COLM_SHARE)
AC_SUBST(LIBCOLM_LA)
AC_SUBST(LIBCOLM_CPPFLAGS)
AC_SUBST(LIBFSM_LA)
AC_SUBST(LIBFSM_CPPFLAGS)

AC_CONFIG_HEADERS([src/colm/config.h src/colm/defs.h src/colm/version.h])
m4_foreach_w([header],
	[
		config.h
		bytecode.h
		defs.h
		debug.h
		pool.h
		input.h
		pdarun.h
		map.h
		type.h
		tree.h
		struct.h
		program.h
		colm.h
		internal.h
		colmex.h
	],
	[AC_CONFIG_LINKS(src/colm/include/colm/header:src/colm/header)]
)
AC_CONFIG_FILES([
	Makefile
	src/aapl/Makefile
	src/libfsm/Makefile
	src/cgil/Makefile
	src/cgil/test/Makefile
	src/colm/Makefile
	src/Makefile
	test/Makefile
	test/harness/Makefile
	test/aapl.d/Makefile
	test/colm.d/Makefile
	test/rlhc.d/Makefile
	test/rlparse.d/Makefile
	test/trans.d/Makefile
	test/ragel.d/Makefile
	doc/Makefile
	doc/colm/Makefile
	doc/ragel/Makefile
	src/ragel/Makefile
	src/ragel/host-asm/Makefile
	src/ragel/host-c/Makefile
	src/ragel/host-crack/Makefile
	src/ragel/host-csharp/Makefile
	src/ragel/host-d/Makefile
	src/ragel/host-go/Makefile
	src/ragel/host-java/Makefile
	src/ragel/host-js/Makefile
	src/ragel/host-julia/Makefile
	src/ragel/host-ocaml/Makefile
	src/ragel/host-ruby/Makefile
	src/ragel/host-rust/Makefile
	src/ragel/host-zig/Makefile
])
AC_OUTPUT

echo "configuration of colm complete"
