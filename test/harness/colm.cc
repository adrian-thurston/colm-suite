/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * colm.d: each case is a colm program followed by sections:
 *
 *   ###### ARGS #####   program arguments
 *   ###### COMP ######  compilation arguments
 *   ###### IN #####     program input
 *   ###### EXP #####    expected output
 *   ###### EXIT ######  expected exit value
 *   ###### HOST ######  host program
 *   ###### CALL ######  file containing C functions
 *
 * ARGS, IN, EXP and EXIT repeat: the Nth of each describes the Nth run of
 * the compiled program. A line ending in --noeol is emitted without its
 * newline.
 */

#include "harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static std::string noEol( const std::string &body )
{
	std::string out;
	size_t pos = 0;
	while ( pos < body.size() ) {
		size_t eol = body.find( '\n', pos );
		if ( eol == std::string::npos )
			eol = body.size();
		std::string line = body.substr( pos, eol - pos );
		if ( hasSuffix( line, "--noeol" ) ) {
			out += stripSuffix( line, "--noeol" );
		}
		else {
			out += line;
			out += "\n";
		}
		pos = eol + 1;
	}
	return out;
}

static bool section( const CaseFile &cf, const char *name, int nth, std::string &out )
{
	const Section *s = cf.find( name, nth );
	if ( s == 0 )
		return false;
	out = noEol( s->body );
	return true;
}

/* Out of tree, cases open their input and include files, and open1 its own
 * source, relative to the build directory they run in. Link them in. */
static void stage( const std::string &src, const std::string &build )
{
	if ( src == build )
		return;
	std::vector<std::string> names;
	listDir( src, names );
	for ( size_t i = 0; i < names.size(); i++ ) {
		if ( hasSuffix( names[i], ".lm" ) || hasSuffix( names[i], ".lmi" ) ||
				hasSuffix( names[i], ".in" ) )
		{
			std::string target = joinPath( build, names[i] );
			if ( !fileExists( target ) ) {
				if ( symlink( joinPath( src, names[i] ).c_str(), target.c_str() ) != 0 )
					fprintf( stderr, "harness: cannot link %s\n", target.c_str() );
			}
		}
	}
}

void enumerateColm( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "colm.d";
	std::string src = config.suiteSrc( suite );
	std::string build = config.suiteBuild( suite );
	std::string wk = config.working( suite );

	if ( !config.list )
		stage( src, build );

	std::vector<std::string> names;
	listDir( src, names );

	for ( size_t i = 0; i < names.size(); i++ ) {
		if ( !hasSuffix( names[i], ".lm" ) )
			continue;
		if ( !sel.selects( names[i] ) )
			continue;

		std::string root = stripSuffix( names[i], ".lm" );
		Job *job = new Job( suite, root );
		job->reportPath = joinPath( wk, root + ".diff" );
		jobs.append( job );

		CaseFile cf;
		std::string err;
		if ( !cf.load( joinPath( src, names[i] ), false, err ) ) {
			job->error( err );
			continue;
		}

		/* Paths are relative to the build directory the steps run in, as the
		 * cases themselves expect. */
		std::string lm = "working/" + root + ".lm";
		std::string bin = "./working/" + root;

		job->steps.push_back( Step::writeFile( joinPath( wk, root + ".lm" ), noEol( cf.preamble ) ) );

		std::string body;
		Words adds;
		if ( section( cf, "CALL", 0, body ) ) {
			job->steps.push_back( Step::writeFile( joinPath( wk, root + ".call.c" ), body ) );
			adds.push_back( "-a" );
			adds.push_back( "working/" + root + ".call.c" );
		}

		bool host = section( cf, "HOST", 0, body );
		if ( host )
			job->steps.push_back( Step::writeFile( joinPath( wk, root + ".host.cc" ), body ) );

		Words comp;
		if ( section( cf, "COMP", 0, body ) )
			comp = splitWords( body );

		if ( host ) {
			std::string parse = "working/" + root + ".parse";
			std::string iface = "working/" + root + ".if";

			Words argv;
			argv.push_back( config.colmBin );
			argv.insert( argv.end(), comp.begin(), comp.end() );
			argv.push_back( "-c" );
			argv.push_back( "-o" );
			argv.push_back( parse + ".c" );
			argv.push_back( "-e" );
			argv.push_back( iface + ".h" );
			argv.push_back( "-x" );
			argv.push_back( iface + ".cc" );
			argv.push_back( lm );
			job->steps.push_back( Step::exec( Step::Compile, argv, build ).capture( CaptureLog ) );

			argv.clear();
			argv.push_back( config.cc );
			argv.push_back( "-c" );
			argv.insert( argv.end(), config.colmCppflags.begin(), config.colmCppflags.end() );
			argv.insert( argv.end(), config.colmLdflags.begin(), config.colmLdflags.end() );
			argv.push_back( "-o" );
			argv.push_back( parse + ".o" );
			argv.push_back( parse + ".c" );
			job->steps.push_back( Step::exec( Step::Compile, argv, build ).capture( CaptureLog ) );

			argv.clear();
			argv.push_back( config.cxx );
			argv.push_back( "-I." );
			argv.insert( argv.end(), config.colmCppflags.begin(), config.colmCppflags.end() );
			argv.insert( argv.end(), config.colmLdflags.begin(), config.colmLdflags.end() );
			argv.push_back( "-o" );
			argv.push_back( "working/" + root );
			argv.push_back( iface + ".cc" );
			argv.push_back( "working/" + root + ".host.cc" );
			argv.push_back( parse + ".o" );
			argv.push_back( "-lcolm" );
			job->steps.push_back( Step::exec( Step::Compile, argv, build ).capture( CaptureLog ) );
		}
		else {
			Words argv;
			argv.push_back( config.colmBin );
			argv.push_back( "-B" );
			argv.push_back( config.topBuilddir );
			argv.insert( argv.end(), comp.begin(), comp.end() );
			argv.insert( argv.end(), adds.begin(), adds.end() );
			argv.push_back( lm );
			job->steps.push_back( Step::exec( Step::Compile, argv, build ).capture( CaptureLog ) );
		}

		/* One run per expected output. With none at all, one run against an
		 * empty expected output. */
		int total = 0;
		for ( int nth = 0; ; nth++ ) {
			if ( cf.find( "EXP", nth ) == 0 && nth > 0 )
				break;
			total++;
			if ( cf.find( "EXP", nth ) == 0 )
				break;
		}

		for ( int nth = 0; nth < total; nth++ ) {
			std::string expected;
			section( cf, "EXP", nth, expected );

			Words args;
			if ( section( cf, "ARGS", nth, body ) )
				args = splitWords( body );

			std::string stdinFile;
			if ( section( cf, "IN", nth, body ) ) {
				char num[32];
				snprintf( num, sizeof(num), "-%d.in", nth );
				stdinFile = joinPath( wk, root + num );
				job->steps.push_back( Step::writeFile( stdinFile, body ) );
			}
			else if ( fileExists( joinPath( src, root + ".in" ) ) ) {
				stdinFile = joinPath( src, root + ".in" );
			}

			int exitValue = 0;
			if ( section( cf, "EXIT", nth, body ) && !trim( body ).empty() )
				exitValue = atoi( trim( body ).c_str() );

			Words argv;
			argv.push_back( bin );
			argv.insert( argv.end(), args.begin(), args.end() );
			job->steps.push_back( Step::exec( Step::Run, argv, build )
					.stdinFrom( stdinFile ).capture( CaptureOutput ).exit( exitValue ) );

			std::string label;
			if ( total > 1 ) {
				char num[32];
				snprintf( num, sizeof(num), "run %d", nth );
				label = num;
			}
			job->steps.push_back( Step::compare( expected, label ) );
		}

		job->artifacts.push_back( joinPath( wk, root ) );
		job->artifacts.push_back( joinPath( wk, root + ".*" ) );
		job->artifacts.push_back( joinPath( wk, root + "-*" ) );
	}
}
