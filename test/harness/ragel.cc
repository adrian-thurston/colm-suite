/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * ragel.d: each case names its host language with @LANG. A case in the
 * language-independent dialect (indep) is first translated into every host
 * language by ./trans. Every resulting program is then run through ragel
 * once per code generation flag, compiled or interpreted, executed, and its
 * output compared against the text after the ##### OUTPUT ##### header.
 *
 * Directives:
 *
 *   @LANG:               indep, or the host language
 *   @PROHIBIT_LANGUAGES: indep only: languages not to translate into
 *   @PROHIBIT_FLAGS:     generation flags not to test, added to the
 *                        language's own list
 *   @ENABLED:            false or no disables the case
 *   @GENERATED:          true marks a generated case; not run
 *   @FILTER:             a shell command the program output is piped through
 *   @RAGEL_FILE:         a file to pass to ragel instead of the extracted case
 */

#include "harness.h"

#include <algorithm>

struct LangOpts
{
	std::string langOpt;
	std::string suffix;
	bool interpreted;
	std::string compiler;
	Words hostRagel;
	Words flags;
	Words libs;
	std::string prohibit;
};

static const char *indepLangs[] = {
	"c", "cg", "cv", "asm", "d", "csharp", "go", "java", "ruby",
	"ocaml", "rust", "crack", "julia", "zig"
};
static const int numIndepLangs = sizeof(indepLangs) / sizeof(indepLangs[0]);

static const char *defaultGenflags[] = {
	"-T0", "-T1", "-F0", "-F1", "-W0", "-W1", "-G0", "-G1", "-G2",
	"-n", "-m", "-e", "--string-tables"
};
static const int numDefaultGenflags = sizeof(defaultGenflags) / sizeof(defaultGenflags[0]);

static Words objcFlags;
static bool objcFlagsLoaded = false;

static bool langOpts( const Config &config, const std::string &src,
		const std::string &lang, LangOpts &lo )
{
	std::string cflags = "-Wall -O3 -I" + src + " -Wno-variadic-macros";

	lo.interpreted = false;
	lo.hostRagel.clear();
	lo.flags.clear();
	lo.libs.clear();

	if ( lang == "c" ) {
		lo.langOpt = "-C"; lo.suffix = "c"; lo.compiler = config.cc;
		lo.hostRagel.push_back( config.ragelBin );
		lo.flags = splitWords( cflags );
		lo.prohibit = "";
	}
	else if ( lang == "cg" ) {
		/* ragel-c, goto based. */
		lo.langOpt = "-C"; lo.suffix = "c"; lo.compiler = config.cc;
		lo.hostRagel.push_back( config.ragelC );
		lo.flags = splitWords( cflags );
		lo.prohibit = "--string-tables";
	}
	else if ( lang == "cv" ) {
		/* ragel-c, var based. */
		lo.langOpt = "-C"; lo.suffix = "c"; lo.compiler = config.cc;
		lo.hostRagel.push_back( config.ragelC );
		lo.hostRagel.push_back( "--var-backend" );
		lo.flags = splitWords( cflags );
		lo.prohibit = "-G0 -G1 -G2 --string-tables";
	}
	else if ( lang == "c++" ) {
		lo.langOpt = "-C"; lo.suffix = "cpp"; lo.compiler = config.cxx;
		lo.hostRagel.push_back( config.ragelBin );
		lo.flags = splitWords( cflags );
		lo.prohibit = "";
	}
	else if ( lang == "obj-c" ) {
		lo.langOpt = "-C"; lo.suffix = "m";
		lo.compiler = config.gnustepConfig.empty() ? "" : config.cc;
		lo.hostRagel.push_back( config.ragelBin );
		if ( !config.gnustepConfig.empty() ) {
			if ( !objcFlagsLoaded ) {
				Words argv;
				argv.push_back( config.gnustepConfig );
				argv.push_back( "--objc-flags" );
				std::string out, err;
				int exitCode;
				if ( runProcess( argv, "", 0, "", &out, 0, exitCode, err ) )
					objcFlags = splitWords( out );
				objcFlagsLoaded = true;
			}
			lo.flags = objcFlags;
		}
		lo.libs = splitWords( "-lobjc -lgnustep-base" );
		lo.prohibit = "";
	}
	else if ( lang == "d" ) {
		lo.langOpt = "-D"; lo.suffix = "d"; lo.compiler = config.dBin;
		lo.hostRagel.push_back( config.ragelD );
		lo.flags = splitWords( "-Wall -O3" );
		lo.prohibit = "--string-tables";
	}
	else if ( lang == "java" ) {
		lo.langOpt = "-J"; lo.suffix = "java"; lo.compiler = config.javacBin;
		lo.hostRagel.push_back( config.ragelJava );
		lo.prohibit = "-G0 -G1 -G2 --string-tables";
	}
	else if ( lang == "ruby" ) {
		lo.langOpt = "-R"; lo.suffix = "rb"; lo.interpreted = true;
		lo.compiler = config.rubyBin;
		lo.hostRagel.push_back( config.ragelRuby );
		lo.prohibit = "-G0 -G1 -G2 --string-tables";
	}
	else if ( lang == "csharp" ) {
		lo.langOpt = "-A"; lo.suffix = "cs"; lo.compiler = config.csharpBin;
		lo.hostRagel.push_back( config.ragelCsharp );
		lo.prohibit = "-G2 --string-tables";
	}
	else if ( lang == "go" ) {
		lo.langOpt = "-Z"; lo.suffix = "go"; lo.compiler = config.goBin;
		lo.hostRagel.push_back( config.ragelGo );
		lo.flags.push_back( "build" );
		lo.prohibit = "--string-tables";
	}
	else if ( lang == "ocaml" ) {
		lo.langOpt = "-O"; lo.suffix = "ml"; lo.interpreted = true;
		lo.compiler = config.ocamlBin;
		lo.hostRagel.push_back( config.ragelOcaml );
		lo.prohibit = "-G0 -G1 -G2 --string-tables";
	}
	else if ( lang == "asm" ) {
		lo.langOpt = "--asm"; lo.suffix = "s"; lo.compiler = config.asmBin;
		lo.hostRagel.push_back( config.ragelAsm );
		lo.flags.push_back( "-no-pie" );
		lo.prohibit = "-T0 -T1 -F0 -F1 -W0 -W1 -G0 -G1 --string-tables";
	}
	else if ( lang == "rust" ) {
		lo.langOpt = "-U"; lo.suffix = "rs"; lo.compiler = config.rustBin;
		lo.hostRagel.push_back( config.ragelRust );
		lo.flags = splitWords( "-A non_upper_case_globals -A dead_code "
				"-A unused_variables -A unused_assignments -A unused_mut -A unused_parens" );
		lo.prohibit = "-G0 -G1 -G2 --string-tables";
	}
	else if ( lang == "zig" ) {
		lo.langOpt = "-B"; lo.suffix = "zig"; lo.compiler = config.zigBin;
		lo.hostRagel.push_back( config.ragelZig );
		lo.flags.push_back( "build-exe" );
		lo.prohibit = "--string-tables";
	}
	else if ( lang == "crack" ) {
		lo.langOpt = "-K"; lo.suffix = "crk"; lo.interpreted = true;
		lo.compiler = config.crackBin;
		lo.hostRagel.push_back( config.ragelCrack );
		lo.prohibit = "-G0 -G1 -G2 --string-tables";
	}
	else if ( lang == "julia" ) {
		lo.langOpt = "-Y"; lo.suffix = "jl"; lo.interpreted = true;
		lo.compiler = config.juliaBin;
		lo.hostRagel.push_back( config.ragelJulia );
		lo.prohibit = "-G0 -G1 -G2 --string-tables";
	}
	else {
		return false;
	}
	return true;
}

/* Runs of dashes become one underscore in the names of generated files. */
static std::string stemOf( const std::string &s )
{
	std::string out;
	for ( size_t i = 0; i < s.size(); i++ ) {
		if ( s[i] == '-' ) {
			if ( out.empty() || out[out.size()-1] != '_' )
				out += '_';
		}
		else {
			out += s[i];
		}
	}
	return out;
}

static bool langSelected( const Config &config, const std::string &lang )
{
	return config.langs.empty() || config.langs.count( lang ) > 0;
}

struct RagelCase
{
	const Config *config;
	std::string src, build, wk;
	std::string casePath;
	std::string expected;
	std::string filter;
	std::string caseProhibit;
};

/* Every generation flag for one program in one host language. */
static void runOptions( const RagelCase &rc, const std::string &lang,
		const std::string &translated, const std::string &lroot,
		int depIdx, JobList &jobs )
{
	const Config &config = *rc.config;
	const char *suite = "ragel.d";

	LangOpts lo;
	if ( !langOpts( config, rc.src, lang, lo ) ) {
		Job *job = new Job( suite, lroot );
		job->error( "unknown language '" + lang + "'" );
		jobs.append( job );
		return;
	}

	if ( !langSelected( config, lang ) )
		return;

	std::string prohibit = lo.prohibit + " " + rc.caseProhibit;

	std::vector<std::string> genflags = config.genflags;
	if ( genflags.empty() )
		genflags.assign( defaultGenflags, defaultGenflags + numDefaultGenflags );

	for ( size_t g = 0; g < genflags.size(); g++ ) {
		const std::string &gen = genflags[g];
		if ( wordIn( gen, prohibit ) )
			continue;

		std::string stem = stemOf( lroot + gen );
		Job *job = new Job( suite, stem );
		job->reportPath = joinPath( rc.wk, stem + ".diff" );
		jobs.append( job );

		if ( lo.compiler.empty() ) {
			job->skip( "no " + lang + " compiler" );
			continue;
		}

		if ( depIdx >= 0 )
			job->deps.push_back( depIdx );

		std::string code = "working/" + stem + "." + lo.suffix;
		std::string binary = "working/" + stem + ".bin";
		std::string classname = stem;

		Words argv = lo.hostRagel;
		argv.push_back( "-I" + rc.src );
		argv.push_back( gen );
		argv.push_back( "-o" );
		argv.push_back( code );
		argv.push_back( translated );
		job->steps.push_back( Step::exec( Step::Prepare, argv, rc.build ).capture( CaptureLog ) );

		if ( lang == "java" ) {
			job->steps.push_back( Step::editFile( joinPath( rc.wk, stem + ".java" ),
					lroot, classname ) );
		}

		if ( !lo.interpreted ) {
			argv.clear();
			argv.push_back( lo.compiler );
			argv.insert( argv.end(), lo.flags.begin(), lo.flags.end() );
			if ( lang == "csharp" )
				argv.push_back( "-out:" + binary );
			else if ( lang == "zig" )
				argv.push_back( "-femit-bin=" + binary );
			else if ( lang != "java" ) {
				argv.push_back( "-o" );
				argv.push_back( binary );
			}
			argv.push_back( code );
			argv.insert( argv.end(), lo.libs.begin(), lo.libs.end() );
			job->steps.push_back( Step::exec( Step::Compile, argv, rc.build ).capture( CaptureLog ) );
		}

		argv.clear();
		if ( lang == "java" ) {
			argv.push_back( "java" );
			argv.push_back( "-classpath" );
			argv.push_back( "working" );
			argv.push_back( classname );
		}
		else if ( lang == "ruby" ) {
			argv.push_back( "ruby" );
			argv.push_back( code );
		}
		else if ( lang == "csharp" ) {
			argv.push_back( "mono" );
			argv.push_back( binary );
		}
		else if ( lang == "ocaml" ) {
			argv.push_back( "ocaml" );
			argv.push_back( code );
		}
		else if ( lang == "crack" ) {
			argv.push_back( config.crackBin );
			argv.push_back( code );
		}
		else if ( lang == "julia" ) {
			argv.push_back( config.juliaBin );
			argv.push_back( code );
		}
		else {
			argv.push_back( "./" + binary );
		}
		job->steps.push_back( Step::exec( Step::Run, argv, rc.build )
				.capture( CaptureOutput ).exit( -1 ) );

		if ( !rc.filter.empty() )
			job->steps.push_back( Step::filter( rc.filter, rc.build ) );

		job->steps.push_back( Step::compare( rc.expected, "" ).trailingCr() );

		job->artifacts.push_back( joinPath( rc.wk, stem + ".*" ) );
		job->artifacts.push_back( joinPath( rc.wk, stem + "$*" ) );
	}
}

void enumerateRagel( const Config &config, const Selection &sel, JobList &jobs )
{
	const char *suite = "ragel.d";
	RagelCase rc;
	rc.config = &config;
	rc.src = config.suiteSrc( suite );
	rc.build = config.suiteBuild( suite );
	rc.wk = config.working( suite );

	std::vector<std::string> names;
	listDir( rc.src, names );

	for ( size_t i = 0; i < names.size(); i++ ) {
		if ( !hasSuffix( names[i], ".rl" ) )
			continue;
		if ( !sel.selects( names[i] ) )
			continue;

		std::string root = stripSuffix( names[i], ".rl" );
		rc.casePath = joinPath( rc.src, names[i] );

		CaseFile cf;
		std::string err;
		if ( !cf.load( rc.casePath, true, err ) ) {
			Job *job = new Job( suite, root );
			job->error( err );
			jobs.append( job );
			continue;
		}

		std::string enabled = cf.directive( "ENABLED" );
		if ( !enabled.empty() && enabled != "true" && enabled != "yes" ) {
			Job *job = new Job( suite, root );
			job->skip( "disabled" );
			jobs.append( job );
			continue;
		}

		if ( cf.directive( "GENERATED" ) == "true" ) {
			Job *job = new Job( suite, root );
			job->skip( "generated" );
			jobs.append( job );
			continue;
		}

		std::string lang = cf.directive( "LANG" );
		if ( lang.empty() ) {
			Job *job = new Job( suite, root );
			job->error( "@LANG unset" );
			jobs.append( job );
			continue;
		}

		std::string ragelFile = cf.directive( "RAGEL_FILE" );
		rc.filter = cf.directive( "FILTER" );
		rc.caseProhibit = cf.directive( "PROHIBIT_FLAGS" );
		std::string prohibitLangs = cf.directive( "PROHIBIT_LANGUAGES" );

		const Section *output = cf.find( "OUTPUT" );
		rc.expected = output != 0 ? cf.after( *output ) : std::string();

		if ( lang == "indep" ) {
			for ( int l = 0; l < numIndepLangs; l++ ) {
				std::string tl = indepLangs[l];
				if ( wordIn( tl, prohibitLangs ) )
					continue;
				if ( !langSelected( config, tl ) )
					continue;

				std::string lroot = root + "_" + tl;
				std::string translated = "working/" + lroot + ".rl";

				/* The translation is shared by every flag. */
				Job *tj = new Job( suite, lroot + ".translate" );
				tj->counted = false;
				tj->reportPath = joinPath( rc.wk, lroot + ".translate.diff" );
				Words argv;
				argv.push_back( "./trans" );
				argv.push_back( tl );
				argv.push_back( translated );
				argv.push_back( rc.casePath );
				argv.push_back( lroot );
				tj->steps.push_back( Step::exec( Step::Prepare, argv, rc.build ).capture( CaptureLog ) );
				jobs.append( tj );
				int depIdx = jobs.length() - 1;

				runOptions( rc, tl, translated, lroot, depIdx, jobs );
			}
		}
		else {
			std::string translated;
			std::string lroot;
			int depIdx = -1;

			if ( !ragelFile.empty() ) {
				translated = joinPath( rc.src, ragelFile );
				lroot = stripSuffix( baseName( ragelFile ), ".rl" );
			}
			else {
				translated = "working/" + root + ".rl";
				lroot = root;

				Job *pj = new Job( suite, root + ".extract" );
				pj->counted = false;
				pj->steps.push_back( Step::writeFile( joinPath( rc.wk, root + ".rl" ),
						output != 0 ? cf.before( *output ) : cf.text ) );
				jobs.append( pj );
				depIdx = jobs.length() - 1;
			}

			runOptions( rc, lang, translated, lroot, depIdx, jobs );
		}
	}
}
