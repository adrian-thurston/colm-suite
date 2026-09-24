/*
 * @LANG: js
 */

%%{
	machine js1;

	action begin { neg = 0; val = 0; }
	action see_neg { neg = 1; }
	action add_digit { val = val * 10 + ( fc - 48 ); }
	action finish { if ( neg ) val = -val; }

	main := ( '-' @see_neg | '+' )? ( digit @add_digit )+ '\n' @finish;
}%%

%% write data;

function run( data )
{
	var p = 0;
	var pe = data.length;
	var eof = data.length;
	var cs = 0;
	var neg = 0, val = 0;

	%% write init;
	%% write exec;

	if ( cs >= js1_first_final )
		console.log( "ACCEPT " + val );
	else
		console.log( "FAIL" );
}

run( "1\n" );
run( "-12\n" );
run( "+123\n" );
run( "x\n" );

##### OUTPUT #####
ACCEPT 1
ACCEPT -12
ACCEPT 123
FAIL
