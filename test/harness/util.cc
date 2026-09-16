/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

#include "harness.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>

std::string joinPath( const std::string &dir, const std::string &file )
{
	if ( dir.empty() )
		return file;
	if ( dir[dir.size()-1] == '/' )
		return dir + file;
	return dir + "/" + file;
}

std::string baseName( const std::string &path )
{
	size_t slash = path.rfind( '/' );
	if ( slash == std::string::npos )
		return path;
	return path.substr( slash + 1 );
}

bool hasSuffix( const std::string &name, const char *suffix )
{
	size_t len = strlen( suffix );
	return name.size() >= len &&
			name.compare( name.size() - len, len, suffix ) == 0;
}

std::string stripSuffix( const std::string &name, const char *suffix )
{
	if ( hasSuffix( name, suffix ) )
		return name.substr( 0, name.size() - strlen( suffix ) );
	return name;
}

bool readFile( const std::string &path, std::string &out )
{
	FILE *f = fopen( path.c_str(), "rb" );
	if ( f == 0 )
		return false;
	out.clear();
	char buf[65536];
	size_t n;
	while ( ( n = fread( buf, 1, sizeof(buf), f ) ) > 0 )
		out.append( buf, n );
	bool ok = !ferror( f );
	fclose( f );
	return ok;
}

bool writeFile( const std::string &path, const std::string &content )
{
	FILE *f = fopen( path.c_str(), "wb" );
	if ( f == 0 )
		return false;
	bool ok = fwrite( content.data(), 1, content.size(), f ) == content.size();
	if ( fclose( f ) != 0 )
		ok = false;
	return ok;
}

bool fileExists( const std::string &path )
{
	struct stat st;
	return stat( path.c_str(), &st ) == 0;
}

bool isDir( const std::string &path )
{
	struct stat st;
	return stat( path.c_str(), &st ) == 0 && S_ISDIR( st.st_mode );
}

bool mkdirp( const std::string &path )
{
	if ( isDir( path ) )
		return true;
	size_t slash = path.rfind( '/' );
	if ( slash != std::string::npos && slash > 0 ) {
		if ( !mkdirp( path.substr( 0, slash ) ) )
			return false;
	}
	return mkdir( path.c_str(), 0777 ) == 0 || errno == EEXIST;
}

/* Remove the contents of a directory, leaving the directory. */
void clearDir( const std::string &path )
{
	DIR *d = opendir( path.c_str() );
	if ( d == 0 )
		return;
	struct dirent *ent;
	while ( ( ent = readdir( d ) ) != 0 ) {
		if ( strcmp( ent->d_name, "." ) == 0 || strcmp( ent->d_name, ".." ) == 0 )
			continue;
		std::string child = joinPath( path, ent->d_name );
		struct stat st;
		if ( lstat( child.c_str(), &st ) != 0 )
			continue;
		if ( S_ISDIR( st.st_mode ) ) {
			clearDir( child );
			rmdir( child.c_str() );
		}
		else {
			unlink( child.c_str() );
		}
	}
	closedir( d );
}

bool listDir( const std::string &path, std::vector<std::string> &names )
{
	DIR *d = opendir( path.c_str() );
	if ( d == 0 )
		return false;
	struct dirent *ent;
	while ( ( ent = readdir( d ) ) != 0 ) {
		if ( ent->d_name[0] == '.' )
			continue;
		names.push_back( ent->d_name );
	}
	closedir( d );
	std::sort( names.begin(), names.end() );
	return true;
}

Words splitWords( const std::string &s )
{
	Words words;
	size_t i = 0;
	while ( i < s.size() ) {
		while ( i < s.size() && isspace( (unsigned char)s[i] ) )
			i++;
		size_t start = i;
		while ( i < s.size() && !isspace( (unsigned char)s[i] ) )
			i++;
		if ( i > start )
			words.push_back( s.substr( start, i - start ) );
	}
	return words;
}

std::string joinWords( const Words &w )
{
	std::string s;
	for ( Words::const_iterator i = w.begin(); i != w.end(); i++ ) {
		if ( i != w.begin() )
			s += " ";
		s += *i;
	}
	return s;
}

std::string trim( const std::string &s )
{
	size_t start = 0;
	while ( start < s.size() && isspace( (unsigned char)s[start] ) )
		start++;
	size_t end = s.size();
	while ( end > start && isspace( (unsigned char)s[end-1] ) )
		end--;
	return s.substr( start, end - start );
}

/* Is word one of the whitespace-separated words of list. */
bool wordIn( const std::string &word, const std::string &list )
{
	Words words = splitWords( list );
	return std::find( words.begin(), words.end(), word ) != words.end();
}

void replaceAll( std::string &s, const std::string &from, const std::string &to )
{
	if ( from.empty() )
		return;
	size_t pos = 0;
	while ( ( pos = s.find( from, pos ) ) != std::string::npos ) {
		s.replace( pos, from.size(), to );
		pos += to.size();
	}
}

/* Quote for display in a log, as a shell would need it. */
std::string shellQuote( const std::string &s )
{
	bool plain = !s.empty();
	for ( size_t i = 0; i < s.size(); i++ ) {
		char c = s[i];
		if ( !( isalnum( (unsigned char)c ) || strchr( "-_./=:+,@", c ) != 0 ) ) {
			plain = false;
			break;
		}
	}
	if ( plain )
		return s;
	std::string q = "'";
	for ( size_t i = 0; i < s.size(); i++ ) {
		if ( s[i] == '\'' )
			q += "'\\''";
		else
			q += s[i];
	}
	q += "'";
	return q;
}

bool Selection::selects( const std::string &path ) const
{
	if ( files.empty() )
		return true;
	return files.count( baseName( path ) ) > 0 ||
			files.count( path ) > 0;
}

std::string Config::suiteSrc( const char *suite ) const
{
	return joinPath( srcdir, suite );
}

std::string Config::suiteBuild( const char *suite ) const
{
	return joinPath( builddir, suite );
}

std::string Config::working( const char *suite ) const
{
	return joinPath( suiteBuild( suite ), "working" );
}
