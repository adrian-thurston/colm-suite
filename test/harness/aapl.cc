/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * aapl.d: run each test program and compare its output, ignoring whitespace,
 * against the .exp file of the same name. The programs are listed here rather
 * than found from the .exp files: test_bubblesort has an .exp but its output
 * comes from unseeded random() and is not tested.
 */

#include "harness.h"

static const char *programs[] = {
	"test_allavl", "test_allsort", "test_avlikeyless", "test_avliter",
	"test_avlkeyless", "test_bstmap", "test_bstset", "test_bsttable",
	"test_compare", "test_dlistval", "test_doublelist", "test_insertsort",
	"test_mergesort", "test_quicksort", "test_rope", "test_sbstmap",
	"test_sbstset", "test_sbsttable", "test_string", "test_svector",
	"test_vector"
};
static const int numPrograms = sizeof(programs) / sizeof(programs[0]);

void enumerateAapl( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "aapl.d";
	std::string src = config.suiteSrc( suite );
	std::string build = config.suiteBuild( suite );
	std::string wk = config.working( suite );

	for ( int i = 0; i < numPrograms; i++ ) {
		std::string root = programs[i];
		std::string exp = root + ".exp";
		if ( !sel.selects( root ) && !sel.selects( exp ) )
			continue;

		Job *job = new Job( suite, root );
		job->reportPath = joinPath( wk, root + ".diff" );
		jobs.append( job );

		std::string expected;
		if ( !readFile( joinPath( src, exp ), expected ) ) {
			job->error( "cannot read " + exp );
			continue;
		}

		Words argv;
		argv.push_back( "./" + root );
		job->steps.push_back( Step::exec( Step::Run, argv, build )
				.capture( CaptureOutput ).exit( -1 ) );
		job->steps.push_back( Step::compare( expected, "" ).whitespace() );
	}
}
