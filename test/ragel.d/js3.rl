/*
 * @LANG: js
 *
 * A scanner: ts and te index the token in the string, and fbreak leaves the
 * exec loop with p on the character that stopped it.
 */

%%{
	machine js3;

	main := |*
		[a-z]+ => { console.log( "word " + data.substring( ts, te ) ); };
		[0-9]+ => { console.log( "number " + data.substring( ts, te ) ); };
		';' => { console.log( "stop" ); fbreak; };
		[ \n]+;
	*|;
}%%

%% write data;

function run( data )
{
	var p = 0;
	var pe = data.length;
	var eof = data.length;
	var cs = 0;
	var ts = 0, te = 0, act = 0;

	%% write init;
	%% write exec;

	if ( cs >= js3_first_final )
		console.log( "ACCEPT at " + p );
	else
		console.log( "FAIL at " + p );
}

run( "hello 123 world" );
run( "one 2; three\n" );
run( "four !\n" );

##### OUTPUT #####
word hello
number 123
word world
ACCEPT at 15
word one
number 2
stop
ACCEPT at 6
word four
FAIL at 5
