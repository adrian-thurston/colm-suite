/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

#include "harness.h"

#include <ctype.h>
#include <stdio.h>

struct Lines
{
	std::vector<std::string> text;   /* as printed */
	std::vector<std::string> key;    /* as compared */
	bool noEol;                      /* last line lacks a newline */
};

static std::string normalize( const std::string &line, bool ignoreWs, bool stripCr )
{
	std::string key;
	if ( ignoreWs ) {
		for ( size_t i = 0; i < line.size(); i++ ) {
			if ( !isspace( (unsigned char)line[i] ) )
				key += line[i];
		}
		return key;
	}
	key = line;
	if ( stripCr ) {
		while ( !key.empty() && key[key.size()-1] == '\r' )
			key.erase( key.size() - 1 );
	}
	return key;
}

static void splitLines( const std::string &s, bool ignoreWs, bool stripCr, Lines &lines )
{
	lines.noEol = false;
	size_t pos = 0;
	while ( pos < s.size() ) {
		size_t eol = s.find( '\n', pos );
		if ( eol == std::string::npos ) {
			lines.noEol = true;
			eol = s.size();
		}
		std::string line = s.substr( pos, eol - pos );
		lines.text.push_back( line );
		lines.key.push_back( normalize( line, ignoreWs, stripCr ) );
		pos = eol + 1;
	}
}

bool outputsMatch( const std::string &expected, const std::string &actual,
		bool ignoreWs, bool stripCr )
{
	if ( expected == actual )
		return true;

	Lines e, a;
	splitLines( expected, ignoreWs, stripCr, e );
	splitLines( actual, ignoreWs, stripCr, a );

	if ( e.key.size() != a.key.size() )
		return false;
	for ( size_t i = 0; i < e.key.size(); i++ ) {
		if ( e.key[i] != a.key[i] )
			return false;
	}
	if ( e.noEol != a.noEol && !ignoreWs )
		return false;
	return true;
}

/* One line of the edit script. */
struct Op
{
	char kind;    /* ' ', '-', '+' */
	int e, a;     /* indices into the expected and actual lines, -1 if none */
};

/* Longest common subsequence by dynamic programming on the differing middle,
 * after trimming the common prefix and suffix. */
static void editScript( const Lines &e, const Lines &a, std::vector<Op> &ops )
{
	size_t n = e.key.size(), m = a.key.size();
	size_t prefix = 0;
	while ( prefix < n && prefix < m && e.key[prefix] == a.key[prefix] )
		prefix++;
	size_t suffix = 0;
	while ( suffix < n - prefix && suffix < m - prefix &&
			e.key[n-1-suffix] == a.key[m-1-suffix] )
		suffix++;

	for ( size_t i = 0; i < prefix; i++ ) {
		Op op = { ' ', (int)i, (int)i };
		ops.push_back( op );
	}

	size_t en = n - prefix - suffix, am = m - prefix - suffix;
	const size_t cap = 4000000;

	if ( en > 0 && am > 0 && en * am <= cap ) {
		/* lcs[i][j]: length of the LCS of e[i..], a[j..]. */
		std::vector<int> lcs( ( en + 1 ) * ( am + 1 ), 0 );
		#define L(i,j) lcs[ (i) * ( am + 1 ) + (j) ]
		for ( size_t i = en; i-- > 0; ) {
			for ( size_t j = am; j-- > 0; ) {
				if ( e.key[prefix+i] == a.key[prefix+j] )
					L(i,j) = L(i+1,j+1) + 1;
				else
					L(i,j) = L(i+1,j) > L(i,j+1) ? L(i+1,j) : L(i,j+1);
			}
		}
		size_t i = 0, j = 0;
		while ( i < en && j < am ) {
			if ( e.key[prefix+i] == a.key[prefix+j] ) {
				Op op = { ' ', (int)(prefix+i), (int)(prefix+j) };
				ops.push_back( op );
				i++; j++;
			}
			else if ( L(i+1,j) >= L(i,j+1) ) {
				Op op = { '-', (int)(prefix+i), -1 };
				ops.push_back( op );
				i++;
			}
			else {
				Op op = { '+', -1, (int)(prefix+j) };
				ops.push_back( op );
				j++;
			}
		}
		for ( ; i < en; i++ ) {
			Op op = { '-', (int)(prefix+i), -1 };
			ops.push_back( op );
		}
		for ( ; j < am; j++ ) {
			Op op = { '+', -1, (int)(prefix+j) };
			ops.push_back( op );
		}
		#undef L
	}
	else {
		/* Too large to align, or one side is empty: replace outright. */
		for ( size_t i = 0; i < en; i++ ) {
			Op op = { '-', (int)(prefix+i), -1 };
			ops.push_back( op );
		}
		for ( size_t j = 0; j < am; j++ ) {
			Op op = { '+', -1, (int)(prefix+j) };
			ops.push_back( op );
		}
	}

	for ( size_t i = 0; i < suffix; i++ ) {
		Op op = { ' ', (int)(n-suffix+i), (int)(m-suffix+i) };
		ops.push_back( op );
	}
}

static void appendLine( std::string &out, char kind, const Lines &lines, int idx )
{
	out += kind;
	out += lines.text[idx];
	out += "\n";
	if ( (size_t)idx == lines.text.size() - 1 && lines.noEol )
		out += "\\ No newline at end of file\n";
}

std::string unifiedDiff( const std::string &expected, const std::string &actual,
		bool ignoreWs, bool stripCr,
		const std::string &expLabel, const std::string &actLabel )
{
	Lines e, a;
	splitLines( expected, ignoreWs, stripCr, e );
	splitLines( actual, ignoreWs, stripCr, a );

	std::vector<Op> ops;
	editScript( e, a, ops );

	/* A trailing newline difference alone. */
	if ( e.noEol != a.noEol && !ops.empty() && ops.back().kind == ' ' ) {
		Op last = ops.back();
		ops.pop_back();
		Op del = { '-', last.e, -1 };
		Op add = { '+', -1, last.a };
		ops.push_back( del );
		ops.push_back( add );
	}

	std::string out;
	out += "--- " + expLabel + "\n";
	out += "+++ " + actLabel + "\n";

	const int context = 3;
	size_t i = 0;
	while ( i < ops.size() ) {
		/* Find the next change. */
		while ( i < ops.size() && ops[i].kind == ' ' )
			i++;
		if ( i >= ops.size() )
			break;

		size_t start = i >= (size_t)context ? i - context : 0;

		/* Extend over changes separated by at most 2*context lines. */
		size_t end = i;
		size_t lastChange = i;
		while ( end < ops.size() ) {
			if ( ops[end].kind != ' ' )
				lastChange = end;
			else if ( end - lastChange > (size_t)( 2 * context ) )
				break;
			end++;
		}
		size_t stop = lastChange + context + 1;
		if ( stop > ops.size() )
			stop = ops.size();

		/* Hunk header: the first line numbers, 1-based, and the counts. */
		int eStart = 0, aStart = 0, eCount = 0, aCount = 0;
		bool eSet = false, aSet = false;
		for ( size_t k = start; k < stop; k++ ) {
			if ( ops[k].e >= 0 ) {
				if ( !eSet ) { eStart = ops[k].e + 1; eSet = true; }
				eCount++;
			}
			if ( ops[k].a >= 0 ) {
				if ( !aSet ) { aStart = ops[k].a + 1; aSet = true; }
				aCount++;
			}
		}
		if ( !eSet )
			eStart = start > 0 ? ops[start-1].e + 1 : 0;
		if ( !aSet )
			aStart = start > 0 ? ops[start-1].a + 1 : 0;

		char header[128];
		snprintf( header, sizeof(header), "@@ -%d,%d +%d,%d @@\n",
				eStart, eCount, aStart, aCount );
		out += header;

		for ( size_t k = start; k < stop; k++ ) {
			if ( ops[k].kind == '+' )
				appendLine( out, '+', a, ops[k].a );
			else
				appendLine( out, ops[k].kind, e, ops[k].e );
		}

		i = stop;
	}
	return out;
}
