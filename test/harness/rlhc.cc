/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * rlhc.d: translate each case/X.in with rlhc for the language encoded in the
 * file name and compare against case/X.exp. The translator takes an output
 * file; it is given /dev/stdout so the result is captured directly.
 */

#include "harness.h"

static const char *rlhcLang( const std::string &file )
{
	/* The letter between the first and second '-'. */
	size_t first = file.find( '-' );
	if ( first == std::string::npos )
		return 0;
	size_t second = file.find( '-', first + 1 );
	std::string code = file.substr( first + 1,
			second == std::string::npos ? std::string::npos : second - first - 1 );

	if ( code == "A" ) return "csharp";
	if ( code == "C" ) return "c";
	if ( code == "J" ) return "java";
	if ( code == "K" ) return "crack";
	if ( code == "O" ) return "ocaml";
	if ( code == "R" ) return "ruby";
	if ( code == "U" ) return "rust";
	if ( code == "Y" ) return "julia";
	if ( code == "Z" ) return "go";
	return 0;
}

void enumerateRlhc( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "rlhc.d";
	std::string src = config.suiteSrc( suite );
	std::string build = config.suiteBuild( suite );
	std::string wk = config.working( suite );
	std::string caseDir = joinPath( src, "case" );

	std::vector<std::string> names;
	listDir( caseDir, names );

	for ( size_t i = 0; i < names.size(); i++ ) {
		if ( !hasSuffix( names[i], ".in" ) )
			continue;
		std::string root = stripSuffix( names[i], ".in" );
		if ( !sel.selects( names[i] ) && !sel.selects( "case/" + names[i] ) )
			continue;

		Job *job = new Job( suite, root );
		job->reportPath = joinPath( wk, root + ".diff" );
		jobs.append( job );

		const char *lang = rlhcLang( names[i] );
		if ( lang == 0 ) {
			job->error( "no language code in file name" );
			continue;
		}

		std::string expected;
		if ( !readFile( joinPath( caseDir, root + ".exp" ), expected ) ) {
			job->error( "cannot read " + root + ".exp" );
			continue;
		}

		Words argv;
		argv.push_back( "./rlhc" );
		argv.push_back( "/dev/stdout" );
		argv.push_back( joinPath( caseDir, names[i] ) );
		argv.push_back( lang );
		job->steps.push_back( Step::exec( Step::Run, argv, build )
				.capture( CaptureOutput ).exit( -1 ) );
		job->steps.push_back( Step::compare( expected, "" ) );
	}
}
