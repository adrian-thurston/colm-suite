#
# libfsm: state machine construction and the direct code generators for C, asm
# and cgil (the code generation intermediate language). It is useful on its own
# for building code generators supporting multiple langauges that are not
# connected to the ragel language.
#

set(LIBFSM_HDR
	action.h fsmgraph.h common.h
	gendata.h redfsm.h dot.h asm.h ragel.h
	binary.h bingoto.h binbreak.h binvar.h
	flat.h flatgoto.h flatbreak.h flatvar.h
	switch.h switchgoto.h switchbreak.h switchvar.h
	goto.h gotoloop.h gotoexp.h
	parsedata.h idbase.h codegen.h
	tables.h
	actloop.h actexp.h
	ipgoto.h)

# The library is assembled from two halves: libfsmgraph, the state machine
# construction library, and the reduction and code generation sources. Colm
# builds its token scanners on libfsmgraph and links it statically, so the
# colm binary never depends on an installed libfsm.
add_library(libfsmgraph STATIC
	idbase.cc fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc fsmgraph.cc
	fsmap.cc fsmcond.cc fsmnfa.cc)

add_library(libfsm
	redfsm.cc gendata.cc
	codegen.cc
	actexp.cc binvar.cc
	tables.cc tabgoto.cc tabbreak.cc tabvar.cc
	binary.cc bingoto.cc binbreak.cc actloop.cc
	flat.cc flatgoto.cc flatbreak.cc flatvar.cc
	switch.cc switchgoto.cc switchbreak.cc switchvar.cc
	goto.cc gotoloop.cc gotoexp.cc ipgoto.cc
	dot.cc asm.cc
	$<TARGET_OBJECTS:libfsmgraph>)

# Consumers include the headers as <libfsm/...>, so the parent of this
# directory is the include root in the build tree, as ${includedir} is once
# installed.
get_filename_component(_libfsm_include_root "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)

foreach(_lib libfsmgraph libfsm)
	target_include_directories(${_lib}
		PUBLIC
		$<BUILD_INTERFACE:${_libfsm_include_root}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
		$<BUILD_INTERFACE:${COLM_AAPL_INCLUDE_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/aapl>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libfsm>)
endforeach()

# The graph objects also go into libfsm, which may be shared.
set_target_properties(libfsmgraph PROPERTIES
	OUTPUT_NAME fsmgraph
	POSITION_INDEPENDENT_CODE ON)

# libfsm carries its own version, independent of the suite's. This mirrors
# the libtool -release string in Makefile.am.
set_target_properties(libfsm PROPERTIES
	OUTPUT_NAME fsm
	VERSION ${LIBFSM_VERSION_NUMERIC}
	SOVERSION ${LIBFSM_VERSION_MAJOR})

add_library(ragel::libfsm ALIAS libfsm)

if(COLM_INSTALL_RAGEL)
	set(_libfsm_install_hdr)
	foreach(_hdr ${LIBFSM_HDR})
		list(APPEND _libfsm_install_hdr "${CMAKE_CURRENT_LIST_DIR}/${_hdr}")
	endforeach()

	install(FILES ${_libfsm_install_hdr}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libfsm")

	install(TARGETS libfsm
		EXPORT ragel-targets
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()
