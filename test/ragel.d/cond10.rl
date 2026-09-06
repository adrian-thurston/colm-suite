/*
 * @LANG: c++
 * @ENABLED: false
 *
 * This test case exercises repetition of a machine that accepts zero-length
 * string. It is very ambiguous and not useful as a pattern.
 *
 * Disabled: the case was imported with a host program that did not compile
 * and never called test(), so it was never run. With the program completed,
 * the machine goes to the error state at the first space after a word, on
 * every input, so the nested condstar needs investigating before an expected
 * output can be recorded.
 */

#include <iostream>
#include <string>
#include <string.h>

using std::cout;
using std::endl;

%%{
	machine foo;
	alphtype char;

	action bs_b { bs_b( src, p ) }
	action bol { p == src }
	action eol { p+1 == eof }

	b = '' %when bs_b;
	B = '' %when !bs_b;
	bol = '' %when bol;
	eol = '' %when eol;

	# {1,25}
	action ini_4 { q_4 = 0; }
	action inc_4 { q_4++; }
	action min_4 { q_4 >= 1 }
	action max_4 { q_4 < 25 }

	# {1,5}
	action ini_5 { q_5 = 0; }
	action inc_5 { q_5++; }
	action min_5 { q_5 >= 1 }
	action max_5 { q_5 < 5 }

	# {100}
	action ini_6 { q_6 = 0; }
	action inc_6 { q_6++; }
	action min_6 { q_6 >= 100 }
	action max_6 { q_6 < 100 }

	R5306833741170350 =
		( '<'  47  's' 't' 'y' 'l' 'e' '>' '<' 
			(:condstar( (  ( (:condstar( (  [a-zA-Z0-9_]  ), ini_4, inc_4, min_4, max_4 ): ) 
			(:condstar( ( ' ' ), ini_5, inc_5, min_5, max_5 ): ) )  ), ini_6, inc_6, min_6, max_6 ): ) )  
	:> any @{ match = 1; };

	main := R5306833741170350;
}%%

%% write data;

bool bs_b( const char *src, const char *p )
{
	return p > src && p[-1] == '\\';
}

void test( const char *str )
{
	int cs = foo_start;
	const char *src = str;
	const char *p = str;
	const char *pe = str + strlen( str );
	const char *eof = pe;
	int match = 0;

	long q_4 = 0, q_5 = 0, q_6 = 0;

	cout << "run:" << endl;
	%% write exec;
	cout << "  stopped at " << ( p - str ) << " of " << ( pe - str ) << ", cs " << cs << ( cs == foo_error ? " (error)" : "" ) << endl;
	if ( match )
		cout << "  success" << endl;
	else
		cout << "  failure" << endl;
	cout << endl;
}

/* Exactly 100 repetitions of a word of 1 to 25 characters followed by 1 to 5
 * spaces, then any character. */
int main()
{
	std::string ok = "</style><";
	for ( int i = 0; i < 100; i++ )
		ok += "abc ";
	ok += "x";
	test( ok.c_str() );

	std::string few = "</style><";
	for ( int i = 0; i < 99; i++ )
		few += "abc ";
	few += "x";
	test( few.c_str() );

	std::string many = "</style><";
	for ( int i = 0; i < 101; i++ )
		many += "abc ";
	many += "x";
	test( many.c_str() );

	test( "</style><abc      x" );
	test( "</style>" );
	return 0;
}

##### OUTPUT #####
