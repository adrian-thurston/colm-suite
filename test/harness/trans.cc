/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * trans.d: translate each language-independent case/X.rl into every host
 * language and compare against the checked-in case/X_lang.rl. The translator
 * takes an output file; it is given /dev/stdout so the result is captured
 * directly.
 */

#include "harness.h"

void enumerateTrans( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "trans.d";
	std::string src = config.suiteSrc( suite );
	std::string build = config.suiteBuild( suite );
	std::string wk = config.working( suite );
	std::string caseDir = joinPath( src, "case" );

	static const char *langs[] = {
		"asm", "crack", "c", "cs", "d", "go", "java", "julia", "ocaml", "zig"
	};
	const int numLangs = sizeof(langs) / sizeof(langs[0]);

	std::vector<std::string> names;
	listDir( caseDir, names );

	for ( size_t i = 0; i < names.size(); i++ ) {
		/* Translations carry an underscore; sources do not. */
		if ( names[i].find( '_' ) != std::string::npos )
			continue;
		if ( isDir( joinPath( caseDir, names[i] ) ) )
			continue;
		if ( !sel.selects( names[i] ) && !sel.selects( "case/" + names[i] ) )
			continue;

		std::string root = stripSuffix( names[i], ".rl" );
		std::string casePath = joinPath( caseDir, names[i] );

		CaseFile cf;
		std::string err;
		if ( !cf.load( casePath, true, err ) ) {
			Job *job = new Job( suite, root );
			job->error( err );
			jobs.append( job );
			continue;
		}
		std::string prohibit = cf.directive( "PROHIBIT_LANGUAGES" );

		for ( int l = 0; l < numLangs; l++ ) {
			if ( wordIn( langs[l], prohibit ) )
				continue;

			std::string name = root + "_" + langs[l];
			Job *job = new Job( suite, name );
			job->reportPath = joinPath( wk, name + ".diff" );
			jobs.append( job );

			std::string expected;
			if ( !readFile( joinPath( caseDir, name + ".rl" ), expected ) ) {
				job->error( "cannot read " + name + ".rl" );
				continue;
			}

			Words argv;
			argv.push_back( "./trans" );
			argv.push_back( langs[l] );
			argv.push_back( "/dev/stdout" );
			argv.push_back( casePath );
			argv.push_back( name );
			job->steps.push_back( Step::exec( Step::Run, argv, build )
					.capture( CaptureOutput ).exit( -1 ) );
			job->steps.push_back( Step::compare( expected, "" ) );
		}
	}
}
