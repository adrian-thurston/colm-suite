/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * The test harness. Every suite is enumerated into the same description: a
 * Job is a named sequence of Steps, optionally depending on other jobs, and
 * every Step is one of a small set of operations (run a program, filter the
 * captured output through a shell command, write or edit a file, compare the
 * captured output against expected text). The scheduler then runs all jobs
 * from all suites through one work queue.
 */

#ifndef _HARNESS_H
#define _HARNESS_H

#include <string>
#include <vector>
#include <set>

#include "vector.h"

typedef std::vector<std::string> Words;

/*
 * Configuration: build-time values from config.h, adjusted by command line
 * options.
 */
struct Config
{
	/* The test source and build directories (the test/ directories). */
	std::string srcdir;
	std::string builddir;
	std::string topBuilddir;

	std::string cc, cxx;

	/* Colm under test. */
	std::string colmBin;
	Words colmCppflags;
	Words colmLdflags;

	/* Ragel under test: the main binary and the host-language frontends. */
	std::string ragelBin;
	std::string ragelC, ragelD, ragelJava, ragelRuby, ragelCsharp, ragelGo,
			ragelOcaml, ragelAsm, ragelRust, ragelZig, ragelCrack, ragelJulia;

	/* Host language toolchains. Empty when not found by configure. */
	std::string dBin, javacBin, rubyBin, csharpBin, goBin, ocamlBin, rustBin,
			zigBin, crackBin, juliaBin, asmBin, gnustepConfig;

	/* Options. */
	int jobs;
	bool verbose;
	bool keep;
	bool list;
	bool commands;
	std::string tapFile;
	std::vector<std::string> filters;

	/* ragel.d selection. Empty means the default set. */
	std::set<std::string> langs;
	std::vector<std::string> genflags;

	Config()
		: jobs(0), verbose(false), keep(false), list(false), commands(false) {}

	std::string suiteSrc( const char *suite ) const;
	std::string suiteBuild( const char *suite ) const;
	std::string working( const char *suite ) const;
};

/* A selection of cases, by case file basename. Empty selects everything. */
struct Selection
{
	std::set<std::string> files;
	bool selects( const std::string &path ) const;
};

/* Where a program's standard output goes. */
enum Capture
{
	CaptureNone,     /* discarded */
	CaptureOutput,   /* becomes the job's current output, to be compared */
	CaptureLog       /* appended to the job log, with stderr */
};

struct Step
{
	enum Kind { Exec, Filter, WriteFile, EditFile, Compare };

	/* How the failure of an Exec is reported. */
	enum Role { Prepare, Compile, Run };

	Kind kind;
	Role role;

	/* Exec. */
	Words argv;
	std::string cwd;
	std::string stdinFile;
	Capture stdoutTo;
	int expectExit;          /* -1: not checked */

	/* Filter: a shell command the current output is piped through. */
	std::string shell;

	/* WriteFile: content to path. EditFile: replace whole-word from with to
	 * in path. */
	std::string path;
	std::string content;
	std::string from, to;

	/* Compare: the current output against expected. */
	std::string expected;
	std::string label;
	bool ignoreWs;
	bool stripCr;

	Step( Kind kind )
		: kind(kind), role(Run), stdoutTo(CaptureNone),
		expectExit(-1), ignoreWs(false), stripCr(false) {}

	static Step exec( Role role, const Words &argv, const std::string &cwd );
	static Step filter( const std::string &shell, const std::string &cwd );
	static Step writeFile( const std::string &path, const std::string &content );
	static Step editFile( const std::string &path,
			const std::string &from, const std::string &to );
	static Step compare( const std::string &expected, const std::string &label );

	Step &stdinFrom( const std::string &file ) { stdinFile = file; return *this; }
	Step &capture( Capture c ) { stdoutTo = c; return *this; }
	Step &exit( int e ) { expectExit = e; return *this; }
	Step &whitespace() { ignoreWs = true; return *this; }
	Step &trailingCr() { stripCr = true; return *this; }
};

enum Outcome
{
	Pending,
	Pass,
	Fail,     /* the subject under test misbehaved */
	Skip,     /* not run: no toolchain, disabled */
	Error     /* the harness could not run the case */
};

struct Job
{
	std::string suite;
	std::string name;
	std::vector<Step> steps;

	/* Indices of jobs that must pass before this one runs. */
	std::vector<int> deps;

	/* Files to remove after a pass, as glob patterns. */
	std::vector<std::string> artifacts;

	/* False for shared preparation work that is not itself a case. */
	bool counted;

	/* Where the failure report goes. */
	std::string reportPath;

	/* Result. */
	Outcome outcome;
	std::string reason;
	std::string log;
	std::string diff;
	std::string output;
	int exitCode;

	/* Scheduling state. */
	int pending;
	std::vector<int> dependents;

	Job( const std::string &suite, const std::string &name )
		: suite(suite), name(name), counted(true), outcome(Pending),
		exitCode(0), pending(0) {}

	std::string id() const { return suite + "/" + name; }
	void skip( const std::string &why ) { outcome = Skip; reason = why; }
	void error( const std::string &why ) { outcome = Error; reason = why; }
	void fail( const std::string &why );
};

typedef Vector<Job*> JobList;

/*
 * Suites.
 */
struct Suite
{
	const char *name;
	void (*enumerate)( const Config &config, const Selection &sel, JobList &jobs );
};

extern Suite suites[];
extern const int numSuites;

void enumerateAapl( const Config &config, const Selection &sel, JobList &jobs );
void enumerateRlhc( const Config &config, const Selection &sel, JobList &jobs );
void enumerateRlparse( const Config &config, const Selection &sel, JobList &jobs );
void enumerateTrans( const Config &config, const Selection &sel, JobList &jobs );
void enumerateColm( const Config &config, const Selection &sel, JobList &jobs );
void enumerateRagel( const Config &config, const Selection &sel, JobList &jobs );

/*
 * Case files: the common section and directive language.
 *
 * A section header is a line of the form "#### NAME ####". A directive is a
 * line containing "@NAME: value". The text before the first header is the
 * preamble.
 */
struct Section
{
	std::string name;
	std::string body;
	long headerStart;   /* offset of the header line */
	long bodyStart;     /* offset of the line after it */
};

struct CaseFile
{
	std::string path;
	std::string text;
	std::string preamble;
	std::vector<Section> sections;
	std::vector< std::pair<std::string, std::string> > directives;

	/* Anchored headers must start the line, as in ragel.d. Otherwise a header
	 * may appear anywhere in a line and the name is the line with every '#'
	 * and ' ' removed, as in colm.d. */
	bool load( const std::string &path, bool anchored, std::string &err );

	const Section *find( const char *name, int nth = 0 ) const;
	bool has( const char *name ) const { return find( name ) != 0; }

	/* All values of a directive, one per line, as the shell drivers' sed
	 * extraction yields. Empty if absent. */
	std::string directive( const char *key ) const;

	/* The text after a section's header line, to the end of the file, and the
	 * text before the header line. */
	std::string after( const Section &s ) const;
	std::string before( const Section &s ) const;
};

/*
 * Processes.
 */

/* Run argv in cwd. Standard input comes from stdinData if non-null, else
 * from stdinFile if non-empty, else /dev/null. Standard output is appended to
 * stdoutBuf if non-null, else discarded. Standard error is appended to
 * stderrBuf if non-null, else inherited. Returns false if the process could
 * not be started, with errMsg set. The exit code is the exit status, or 128
 * plus the signal number. */
bool runProcess( const Words &argv, const std::string &cwd,
		const std::string *stdinData, const std::string &stdinFile,
		std::string *stdoutBuf, std::string *stderrBuf,
		int &exitCode, std::string &errMsg );

/*
 * Comparison.
 */
bool outputsMatch( const std::string &expected, const std::string &actual,
		bool ignoreWs, bool stripCr );
std::string unifiedDiff( const std::string &expected, const std::string &actual,
		bool ignoreWs, bool stripCr,
		const std::string &expLabel, const std::string &actLabel );

/*
 * Execution.
 */
void runJobs( const Config &config, JobList &jobs );
std::string describeStep( const Step &step );

/*
 * Utilities.
 */
std::string joinPath( const std::string &dir, const std::string &file );
std::string baseName( const std::string &path );
std::string stripSuffix( const std::string &name, const char *suffix );
bool hasSuffix( const std::string &name, const char *suffix );
bool readFile( const std::string &path, std::string &out );
bool writeFile( const std::string &path, const std::string &content );
bool fileExists( const std::string &path );
bool isDir( const std::string &path );
bool mkdirp( const std::string &path );
void clearDir( const std::string &path );
bool listDir( const std::string &path, std::vector<std::string> &names );
Words splitWords( const std::string &s );
std::string joinWords( const Words &w );
std::string trim( const std::string &s );
bool wordIn( const std::string &word, const std::string &list );
void replaceAll( std::string &s, const std::string &from, const std::string &to );
std::string shellQuote( const std::string &s );

#endif
