/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

#include "harness.h"

#include <ctype.h>

/* Match "#+ *[a-zA-Z]+ *#+" starting at pos. */
static bool matchHeaderAt( const std::string &line, size_t pos, std::string &name )
{
	size_t i = pos;
	if ( i >= line.size() || line[i] != '#' )
		return false;
	while ( i < line.size() && line[i] == '#' )
		i++;
	while ( i < line.size() && line[i] == ' ' )
		i++;
	size_t nameStart = i;
	while ( i < line.size() && isalpha( (unsigned char)line[i] ) )
		i++;
	if ( i == nameStart )
		return false;
	size_t nameEnd = i;
	while ( i < line.size() && line[i] == ' ' )
		i++;
	if ( i >= line.size() || line[i] != '#' )
		return false;
	name = line.substr( nameStart, nameEnd - nameStart );
	return true;
}

static bool matchHeader( const std::string &line, bool anchored, std::string &name )
{
	if ( anchored )
		return matchHeaderAt( line, 0, name );

	/* Anywhere in the line, as the colm.d awk pattern. The name is then the
	 * line with every '#' and ' ' removed. */
	for ( size_t pos = 0; pos < line.size(); pos++ ) {
		if ( line[pos] == '#' && matchHeaderAt( line, pos, name ) ) {
			name.clear();
			for ( size_t i = 0; i < line.size(); i++ ) {
				if ( line[i] != '#' && line[i] != ' ' )
					name += line[i];
			}
			return true;
		}
	}
	return false;
}

/* Find "@KEY: value" in a line. */
static bool matchDirective( const std::string &line, std::string &key, std::string &value )
{
	size_t at = 0;
	while ( ( at = line.find( '@', at ) ) != std::string::npos ) {
		size_t i = at + 1;
		while ( i < line.size() && ( isupper( (unsigned char)line[i] ) || line[i] == '_' ) )
			i++;
		if ( i > at + 1 && i < line.size() && line[i] == ':' ) {
			key = line.substr( at + 1, i - at - 1 );
			value = trim( line.substr( i + 1 ) );
			return true;
		}
		at = i;
	}
	return false;
}

bool CaseFile::load( const std::string &p, bool anchored, std::string &err )
{
	path = p;
	if ( !readFile( path, text ) ) {
		err = "cannot read " + path;
		return false;
	}

	preamble.clear();
	sections.clear();
	directives.clear();

	std::string *body = &preamble;
	size_t pos = 0;
	while ( pos < text.size() ) {
		size_t eol = text.find( '\n', pos );
		size_t next = eol == std::string::npos ? text.size() : eol + 1;
		std::string line = text.substr( pos, ( eol == std::string::npos ? text.size() : eol ) - pos );

		std::string name;
		if ( matchHeader( line, anchored, name ) ) {
			Section s;
			s.name = name;
			s.headerStart = pos;
			s.bodyStart = next;
			sections.push_back( s );
			body = &sections.back().body;
		}
		else {
			/* Every line ends in a newline, as awk prints them. */
			body->append( line );
			body->append( "\n" );

			std::string key, value;
			if ( matchDirective( line, key, value ) )
				directives.push_back( std::make_pair( key, value ) );
		}
		pos = next;
	}
	return true;
}

const Section *CaseFile::find( const char *name, int nth ) const
{
	int n = 0;
	for ( size_t i = 0; i < sections.size(); i++ ) {
		if ( sections[i].name == name ) {
			if ( n == nth )
				return &sections[i];
			n++;
		}
	}
	return 0;
}

std::string CaseFile::directive( const char *key ) const
{
	std::string result;
	for ( size_t i = 0; i < directives.size(); i++ ) {
		if ( directives[i].first == key ) {
			if ( !result.empty() )
				result += "\n";
			result += directives[i].second;
		}
	}
	return result;
}

std::string CaseFile::after( const Section &s ) const
{
	if ( (size_t)s.bodyStart >= text.size() )
		return std::string();
	return text.substr( s.bodyStart );
}

std::string CaseFile::before( const Section &s ) const
{
	return text.substr( 0, s.headerStart );
}
