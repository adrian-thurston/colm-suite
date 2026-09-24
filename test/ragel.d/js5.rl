/*
 * @LANG: js
 *
 * fentry and a computed fgoto, with fc read in the same action.
 */

%%{
	machine js5;

	one := 'one\n' @{ console.log( "one" ); };
	two := 'two\n' @{ console.log( "two" ); };
	four := 'four\n' @{ console.log( "four" ); };

	main :=
		( 'hello' | 'there' | 'friend' )
		'\n' @{ var s = fentry(one); if ( fc == 10 ) fgoto *s; }
		( 'one' | 'two' | 'four' ) '\n';
}%%

%% write data;

function run( data )
{
	var p = 0;
	var pe = data.length;
	var cs = 0;

	%% write init;
	%% write exec;

	if ( cs >= js5_first_final )
		console.log( "ACCEPT" );
	else
		console.log( "FAIL" );
}

run( "hello\none\n" );
run( "there\ntwo\n" );
run( "friend\nfour\n" );

##### OUTPUT #####
one
ACCEPT
FAIL
FAIL
