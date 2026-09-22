/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * aapl.d: run each test program and compare its output, ignoring whitespace,
 * against the .exp file of the same name. The stress programs have no expected
 * output: each runs random operations against a structure for a bounded time,
 * verifying as it goes and once more on the way out, and passes by exiting 0.
 * An assertion failure aborts, and the seed it reports on stderr reproduces
 * the run. The programs are listed here rather than found in the directory.
 */

#include <stdio.h>

#include "harness.h"

static const char *programs[] = {
	"test_allavl", "test_allsort", "test_avlikeyless", "test_avliter",
	"test_avlkeyless", "test_bstmap", "test_bstset", "test_bsttable",
	"test_bubblesort", "test_compare", "test_dlistval", "test_doublelist",
	"test_insertsort", "test_mergesort", "test_quicksort", "test_rope",
	"test_sbstmap", "test_sbstset", "test_sbsttable", "test_string",
	"test_svector", "test_vector"
};
static const int numPrograms = sizeof(programs) / sizeof(programs[0]);

static const char *stress[] = {
	"stress_allsort", "stress_avlimap", "stress_avlimel", "stress_avlimelkey",
	"stress_avliset", "stress_avliter", "stress_avlitree", "stress_avlmap",
	"stress_avlmel", "stress_avlmelkey", "stress_avlset", "stress_avltree",
	"stress_stblsort", "stress_svector"
};
static const int numStress = sizeof(stress) / sizeof(stress[0]);

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

	char secs[32];
	snprintf( secs, sizeof(secs), "%d", config.stressSecs );

	for ( int i = 0; i < numStress; i++ ) {
		std::string root = stress[i];
		if ( !sel.selects( root ) )
			continue;

		Job *job = new Job( suite, root );
		job->reportPath = joinPath( wk, root + ".diff" );
		jobs.append( job );

		if ( config.stressSecs <= 0 ) {
			job->skip( "--stress 0" );
			continue;
		}

		Words argv;
		argv.push_back( "./" + root );
		argv.push_back( secs );
		job->steps.push_back( Step::exec( Step::Run, argv, build ) );
	}
}
