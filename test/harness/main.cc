/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

#include "harness.h"
#include "config.h"

#include <fnmatch.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <thread>

Suite suites[] = {
	{ "aapl.d", enumerateAapl },
	{ "colm.d", enumerateColm },
	{ "ragel.d", enumerateRagel },
	{ "rlhc.d", enumerateRlhc },
	{ "rlparse.d", enumerateRlparse },
	{ "trans.d", enumerateTrans },
};
const int numSuites = sizeof(suites) / sizeof(suites[0]);

static void usage()
{
	printf(
"usage: harness [options] [selection ...]\n"
"\n"
"Runs the test suites under test/. A selection is a suite (colm.d), a case\n"
"file in a suite (colm.d/argv1.lm, ragel.d/atoi1.rl, rlhc.d/case/x.in), or,\n"
"when run from inside a suite directory, a bare case file name.\n"
"\n"
"  -j N              run N cases at once (default: MAKEFLAGS, then the CPU count)\n"
"  -v                report every case, not only failures\n"
"  --suite NAME      run only this suite; may be repeated or comma separated\n"
"  --filter GLOB     run only cases whose suite/name matches GLOB\n"
"  --lang L,...      ragel.d: only these host languages\n"
"                    (c cg cv c++ obj-c asm d java ruby csharp go ocaml rust zig crack julia)\n"
"  --genflags F,...  ragel.d: only these generation flags (default: -T0 -T1 -F0 -F1\n"
"                    -W0 -W1 -G0 -G1 -G2 -n -m -e --string-tables)\n"
"  --list            list the selected cases without running them\n"
"  --commands        print the steps of the selected cases without running them\n"
"  --keep            keep the generated files of passing cases\n"
"  --tap FILE        write results in TAP format to FILE\n"
"  --srcdir DIR      the test source directory (default: the configured one)\n"
"  --builddir DIR    the test build directory (default: the configured one)\n"
"\n"
"Results go to working/ under each suite's build directory: a .diff file per\n"
"failing case with the differences and the commands run.\n"
"\n"
"Exit status: 0 all passed, 1 the harness could not run a case, 2 a case failed.\n"
	);
}

static Suite *findSuite( const std::string &name )
{
	for ( int i = 0; i < numSuites; i++ ) {
		if ( name == suites[i].name )
			return &suites[i];
	}
	return 0;
}

/* The -j from MAKEFLAGS when run under make -j. */
static int makeJobs()
{
	const char *mf = getenv( "MAKEFLAGS" );
	if ( mf == 0 )
		return 0;
	Words words = splitWords( mf );
	for ( size_t i = 0; i < words.size(); i++ ) {
		const std::string &w = words[i];
		if ( w.compare( 0, 2, "-j" ) == 0 ) {
			if ( w.size() > 2 )
				return atoi( w.c_str() + 2 );
			if ( i + 1 < words.size() && isdigit( (unsigned char)words[i+1][0] ) )
				return atoi( words[i+1].c_str() );
			return 0;
		}
		if ( w.compare( 0, 7, "--jobs=" ) == 0 )
			return atoi( w.c_str() + 7 );
	}
	return 0;
}

static void splitList( const std::string &s, std::vector<std::string> &out )
{
	size_t pos = 0;
	while ( pos <= s.size() ) {
		size_t comma = s.find( ',', pos );
		if ( comma == std::string::npos )
			comma = s.size();
		if ( comma > pos )
			out.push_back( s.substr( pos, comma - pos ) );
		pos = comma + 1;
	}
}

static std::string currentDir()
{
	char buf[4096];
	if ( getcwd( buf, sizeof(buf) ) == 0 )
		return std::string();
	return buf;
}

/* Keep the jobs matching a filter, and the uncounted preparation jobs they
 * depend on, renumbering dependencies. */
static void applyFilters( const Config &config, JobList &jobs )
{
	if ( config.filters.empty() )
		return;

	std::vector<bool> keep( jobs.length(), false );
	for ( long i = 0; i < jobs.length(); i++ ) {
		Job &job = *jobs[i];
		if ( !job.counted )
			continue;
		std::string id = job.id();
		for ( size_t f = 0; f < config.filters.size(); f++ ) {
			if ( fnmatch( config.filters[f].c_str(), id.c_str(), 0 ) == 0 ) {
				keep[i] = true;
				break;
			}
		}
	}
	for ( long i = 0; i < jobs.length(); i++ ) {
		if ( keep[i] ) {
			for ( size_t d = 0; d < jobs[i]->deps.size(); d++ )
				keep[jobs[i]->deps[d]] = true;
		}
	}

	std::vector<int> renumber( jobs.length(), -1 );
	JobList kept;
	for ( long i = 0; i < jobs.length(); i++ ) {
		if ( keep[i] ) {
			renumber[i] = kept.length();
			kept.append( jobs[i] );
		}
		else {
			delete jobs[i];
		}
	}
	for ( long i = 0; i < kept.length(); i++ ) {
		for ( size_t d = 0; d < kept[i]->deps.size(); d++ )
			kept[i]->deps[d] = renumber[kept[i]->deps[d]];
	}
	jobs.transfer( kept );
}

static void writeTap( const Config &config, const JobList &jobs )
{
	FILE *f = fopen( config.tapFile.c_str(), "w" );
	if ( f == 0 ) {
		fprintf( stderr, "harness: cannot write %s\n", config.tapFile.c_str() );
		return;
	}
	int n = 0;
	for ( long i = 0; i < jobs.length(); i++ ) {
		if ( jobs[i]->counted )
			n++;
	}
	fprintf( f, "TAP version 13\n1..%d\n", n );
	int k = 0;
	for ( long i = 0; i < jobs.length(); i++ ) {
		const Job &job = *jobs[i];
		if ( !job.counted )
			continue;
		k++;
		switch ( job.outcome ) {
			case Pass:
				fprintf( f, "ok %d - %s\n", k, job.id().c_str() );
				break;
			case Skip:
				fprintf( f, "ok %d - %s # SKIP %s\n", k, job.id().c_str(), job.reason.c_str() );
				break;
			default:
				fprintf( f, "not ok %d - %s # %s\n", k, job.id().c_str(), job.reason.c_str() );
				break;
		}
	}
	fclose( f );
}

int main( int argc, char **argv )
{
	signal( SIGPIPE, SIG_IGN );

	Config config;
	config.srcdir = joinPath( HARNESS_TOP_SRCDIR, "test" );
	config.builddir = joinPath( HARNESS_TOP_BUILDDIR, "test" );
	config.topBuilddir = HARNESS_TOP_BUILDDIR;
	config.cc = HARNESS_CC;
	config.cxx = HARNESS_CXX;
	config.colmBin = HARNESS_COLM_BIN;
	config.colmCppflags = splitWords( HARNESS_COLM_CPPFLAGS );
	config.colmLdflags = splitWords( HARNESS_COLM_LDFLAGS );
	config.ragelBin = HARNESS_RAGEL_BIN;
	config.ragelC = HARNESS_RAGEL_C_BIN;
	config.ragelD = HARNESS_RAGEL_D_BIN;
	config.ragelJava = HARNESS_RAGEL_JAVA_BIN;
	config.ragelRuby = HARNESS_RAGEL_RUBY_BIN;
	config.ragelCsharp = HARNESS_RAGEL_CSHARP_BIN;
	config.ragelGo = HARNESS_RAGEL_GO_BIN;
	config.ragelOcaml = HARNESS_RAGEL_OCAML_BIN;
	config.ragelAsm = HARNESS_RAGEL_ASM_BIN;
	config.ragelRust = HARNESS_RAGEL_RUST_BIN;
	config.ragelZig = HARNESS_RAGEL_ZIG_BIN;
	config.ragelCrack = HARNESS_RAGEL_CRACK_BIN;
	config.ragelJulia = HARNESS_RAGEL_JULIA_BIN;
	config.dBin = HARNESS_D_BIN;
	config.javacBin = HARNESS_JAVAC_BIN;
	config.rubyBin = HARNESS_RUBY_BIN;
	config.csharpBin = HARNESS_CSHARP_BIN;
	config.goBin = HARNESS_GO_BIN;
	config.ocamlBin = HARNESS_OCAML_BIN;
	config.rustBin = HARNESS_RUST_BIN;
	config.zigBin = HARNESS_ZIG_BIN;
	config.crackBin = HARNESS_CRACK_BIN;
	config.juliaBin = HARNESS_JULIA_BIN;
	config.asmBin = HARNESS_ASM_BIN;
	config.gnustepConfig = HARNESS_GNUSTEP_CONFIG;

	std::vector<std::string> suiteNames;
	std::vector<std::string> positional;

	for ( int i = 1; i < argc; i++ ) {
		std::string arg = argv[i];
		std::string value;
		bool needValue = arg == "-j" || arg == "--suite" || arg == "--filter" ||
				arg == "--lang" || arg == "--genflags" || arg == "--tap" ||
				arg == "--srcdir" || arg == "--builddir";
		if ( needValue ) {
			if ( i + 1 >= argc ) {
				fprintf( stderr, "harness: %s needs a value\n", arg.c_str() );
				return 1;
			}
			value = argv[++i];
		}

		if ( arg == "-h" || arg == "--help" ) {
			usage();
			return 0;
		}
		else if ( arg == "-v" )
			config.verbose = true;
		else if ( arg == "--list" )
			config.list = true;
		else if ( arg == "--commands" )
			config.list = config.commands = true;
		else if ( arg == "--keep" )
			config.keep = true;
		else if ( arg == "-j" )
			config.jobs = atoi( value.c_str() );
		else if ( arg.compare( 0, 2, "-j" ) == 0 )
			config.jobs = atoi( arg.c_str() + 2 );
		else if ( arg == "--suite" )
			splitList( value, suiteNames );
		else if ( arg == "--filter" )
			config.filters.push_back( value );
		else if ( arg == "--lang" ) {
			std::vector<std::string> langs;
			splitList( value, langs );
			config.langs.insert( langs.begin(), langs.end() );
		}
		else if ( arg == "--genflags" )
			splitList( value, config.genflags );
		else if ( arg == "--tap" )
			config.tapFile = value;
		else if ( arg == "--srcdir" )
			config.srcdir = value;
		else if ( arg == "--builddir" )
			config.builddir = value;
		else if ( arg.size() > 1 && arg[0] == '-' ) {
			fprintf( stderr, "harness: unknown option %s\n", arg.c_str() );
			return 1;
		}
		else
			positional.push_back( arg );
	}

	if ( config.jobs <= 0 )
		config.jobs = makeJobs();
	if ( config.jobs <= 0 )
		config.jobs = std::thread::hardware_concurrency();
	if ( config.jobs <= 0 )
		config.jobs = 2;

	/* Inside a suite directory, bare names are its case files. */
	std::string here = baseName( currentDir() );
	Suite *hereSuite = findSuite( here );

	/* Selections per suite. */
	std::vector<Selection> selections( numSuites );
	std::vector<bool> selected( numSuites, false );

	for ( size_t i = 0; i < suiteNames.size(); i++ ) {
		Suite *s = findSuite( suiteNames[i] );
		if ( s == 0 ) {
			fprintf( stderr, "harness: unknown suite %s\n", suiteNames[i].c_str() );
			return 1;
		}
		selected[s - suites] = true;
	}

	for ( size_t i = 0; i < positional.size(); i++ ) {
		std::string arg = positional[i];
		while ( arg.size() > 1 && arg[arg.size()-1] == '/' )
			arg.erase( arg.size() - 1 );

		Suite *s = findSuite( arg );
		if ( s != 0 ) {
			selected[s - suites] = true;
			continue;
		}

		size_t slash = arg.find( '/' );
		if ( slash != std::string::npos && findSuite( arg.substr( 0, slash ) ) != 0 ) {
			s = findSuite( arg.substr( 0, slash ) );
			selected[s - suites] = true;
			selections[s - suites].files.insert( arg.substr( slash + 1 ) );
			selections[s - suites].files.insert( baseName( arg ) );
			continue;
		}

		if ( hereSuite != 0 ) {
			selected[hereSuite - suites] = true;
			selections[hereSuite - suites].files.insert( arg );
			selections[hereSuite - suites].files.insert( baseName( arg ) );
			continue;
		}

		fprintf( stderr, "harness: %s is not a suite or suite/case\n", arg.c_str() );
		return 1;
	}

	bool any = false;
	for ( int i = 0; i < numSuites; i++ )
		any = any || selected[i];
	if ( !any ) {
		if ( hereSuite != 0 )
			selected[hereSuite - suites] = true;
		else {
			for ( int i = 0; i < numSuites; i++ )
				selected[i] = true;
		}
	}

	/* Enumerate. */
	JobList jobs;
	for ( int i = 0; i < numSuites; i++ ) {
		if ( !selected[i] )
			continue;
		std::string wk = config.working( suites[i].name );
		if ( !config.list ) {
			if ( !mkdirp( wk ) ) {
				fprintf( stderr, "harness: cannot create %s\n", wk.c_str() );
				return 1;
			}
			clearDir( wk );
		}
		suites[i].enumerate( config, selections[i], jobs );
	}
	applyFilters( config, jobs );

	if ( config.commands ) {
		for ( long i = 0; i < jobs.length(); i++ ) {
			const Job &job = *jobs[i];
			printf( "==== %s%s\n", job.id().c_str(), job.counted ? "" : " (preparation)" );
			if ( job.outcome == Skip ) {
				printf( "skip: %s\n", job.reason.c_str() );
				continue;
			}
			for ( size_t d = 0; d < job.deps.size(); d++ )
				printf( "after %s\n", jobs[job.deps[d]]->id().c_str() );
			for ( size_t s = 0; s < job.steps.size(); s++ )
				printf( "%s", describeStep( job.steps[s] ).c_str() );
		}
		return 0;
	}

	if ( config.list ) {
		for ( long i = 0; i < jobs.length(); i++ ) {
			if ( !jobs[i]->counted )
				continue;
			if ( jobs[i]->outcome == Skip )
				printf( "%s # skip: %s\n", jobs[i]->id().c_str(), jobs[i]->reason.c_str() );
			else
				printf( "%s\n", jobs[i]->id().c_str() );
		}
		return 0;
	}

	struct timespec start, end;
	clock_gettime( CLOCK_MONOTONIC, &start );

	runJobs( config, jobs );

	clock_gettime( CLOCK_MONOTONIC, &end );
	double elapsed = ( end.tv_sec - start.tv_sec ) + ( end.tv_nsec - start.tv_nsec ) / 1e9;

	/* Summary, in enumeration order. */
	int cases = 0, failures = 0, skipped = 0, errors = 0;
	for ( long i = 0; i < jobs.length(); i++ ) {
		const Job &job = *jobs[i];
		switch ( job.outcome ) {
			case Pass:
				if ( job.counted )
					cases++;
				break;
			case Fail:
				if ( job.counted ) {
					cases++;
					failures++;
				}
				break;
			case Skip:
				if ( job.counted )
					skipped++;
				break;
			case Error:
			case Pending:
				errors++;
				break;
		}
	}

	if ( failures > 0 ) {
		printf( "---- failed\n" );
		for ( long i = 0; i < jobs.length(); i++ ) {
			const Job &job = *jobs[i];
			if ( job.counted && job.outcome == Fail )
				printf( "%s: %s\n", job.id().c_str(), job.reason.c_str() );
		}
	}
	if ( errors > 0 ) {
		printf( "---- internal errors\n" );
		for ( long i = 0; i < jobs.length(); i++ ) {
			const Job &job = *jobs[i];
			if ( job.outcome == Error || job.outcome == Pending )
				printf( "%s: %s\n", job.id().c_str(), job.reason.c_str() );
		}
	}

	printf( "---- cases\n%d\n", cases );
	printf( "---- failures\n%d\n", failures );
	if ( skipped > 0 )
		printf( "---- skipped\n%d\n", skipped );
	printf( "---- time\n%.1fs\n", elapsed );

	if ( !config.tapFile.empty() )
		writeTap( config, jobs );

	if ( errors > 0 )
		return 1;
	if ( failures > 0 )
		return 2;
	return 0;
}
