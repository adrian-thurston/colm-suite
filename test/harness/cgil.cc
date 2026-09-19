/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * cgil.d: translate each case/X.ri with the cgil translator for the language
 * encoded in the file name and compare against the expected file beside it,
 * which carries the language as its extension. The translator takes an output
 * file; it is given /dev/stdout so the result is captured directly.
 */

#include "harness.h"

/* The field between the first and second '-': test-go-02.ri is go. */
static std::string cgilLang( const std::string &file )
{
	size_t first = file.find( '-' );
	if ( first == std::string::npos )
		return std::string();
	size_t second = file.find( '-', first + 1 );
	if ( second == std::string::npos )
		return std::string();
	return file.substr( first + 1, second - first - 1 );
}

void enumerateCgil( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "cgil.d";
	std::string src = config.suiteSrc( suite );
	std::string build = config.suiteBuild( suite );
	std::string wk = config.working( suite );
	std::string caseDir = joinPath( src, "case" );

	std::vector<std::string> names;
	listDir( caseDir, names );

	for ( size_t i = 0; i < names.size(); i++ ) {
		if ( !hasSuffix( names[i], ".ri" ) )
			continue;
		std::string root = stripSuffix( names[i], ".ri" );
		if ( !sel.selects( names[i] ) && !sel.selects( "case/" + names[i] ) )
			continue;

		Job *job = new Job( suite, root );
		job->reportPath = joinPath( wk, root + ".diff" );
		jobs.append( job );

		std::string lang = cgilLang( names[i] );
		if ( lang.empty() ) {
			job->error( "no language in file name" );
			continue;
		}

		std::string expected;
		if ( !readFile( joinPath( caseDir, root + "." + lang ), expected ) ) {
			job->error( "cannot read " + root + "." + lang );
			continue;
		}

		Words argv;
		argv.push_back( joinPath( config.cgilDir, "cgil-" + lang ) );
		argv.push_back( joinPath( caseDir, names[i] ) );
		argv.push_back( "/dev/stdout" );
		/* A translator that fails must say so in its exit status. The old
		 * runtests ran under set -e and relied on that. */
		job->steps.push_back( Step::exec( Step::Run, argv, build )
				.capture( CaptureOutput ).exit( 0 ) );
		job->steps.push_back( Step::compare( expected, "" ) );
	}
}
