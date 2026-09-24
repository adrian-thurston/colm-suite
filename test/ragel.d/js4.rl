/*
 * @LANG: js
 *
 * fcall and fret through a stack the host declares.
 */

%%{
	machine js4;

	inner := 'b'+ '\n' @{ console.log( "ret" ); fret; };

	main := ( 'a' @{ console.log( "call" ); fcall inner; } )+ 'c' '\n';
}%%

%% write data;

function run( data )
{
	var p = 0;
	var pe = data.length;
	var cs = 0;
	var stack = new Array( 8 );
	var top = 0;

	%% write init;
	%% write exec;

	if ( cs >= js4_first_final )
		console.log( "ACCEPT" );
	else
		console.log( "FAIL" );
}

run( "ab\nabbb\nc\n" );
run( "ab\nc\nc\n" );
run( "c\n" );

##### OUTPUT #####
call
ret
call
ret
ACCEPT
call
ret
FAIL
FAIL
