/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * rlparse.d: parse each case/X.rl with both frontends, from inside the case
 * directory so the file name in the output is bare, and compare against the
 * .exp for that frontend.
 */

#include "harness.h"

void enumerateRlparse( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "rlparse.d";
	std::string src = config.suiteSrc( suite );
	std::string build = config.suiteBuild( suite );
	std::string wk = config.working( suite );
	std::string caseDir = joinPath( src, "case" );

	static const char *frontends[] = { "--reduce-frontend", "--colm-frontend" };

	std::vector<std::string> names;
	listDir( caseDir, names );

	for ( size_t i = 0; i < names.size(); i++ ) {
		if ( !hasSuffix( names[i], ".rl" ) )
			continue;
		std::string root = stripSuffix( names[i], ".rl" );
		if ( !sel.selects( names[i] ) && !sel.selects( "case/" + names[i] ) )
			continue;

		for ( int f = 0; f < 2; f++ ) {
			std::string name = root + frontends[f];
			Job *job = new Job( suite, name );
			job->reportPath = joinPath( wk, name + ".diff" );
			jobs.append( job );

			std::string expected;
			if ( !readFile( joinPath( caseDir, name + ".exp" ), expected ) ) {
				job->error( "cannot read " + name + ".exp" );
				continue;
			}

			Words argv;
			argv.push_back( joinPath( build, "rlparse" ) );
			argv.push_back( frontends[f] );
			argv.push_back( names[i] );
			job->steps.push_back( Step::exec( Step::Run, argv, caseDir )
					.capture( CaptureOutput ).exit( -1 ) );
			job->steps.push_back( Step::compare( expected, "" ) );
		}
	}
}
