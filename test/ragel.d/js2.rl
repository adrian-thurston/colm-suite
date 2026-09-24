/*
 * @LANG: js
 *
 * Host language scanning. Comments and string literals carry the section
 * delimiters and braces, and none of them may be taken for ragel input.
 */

// %%{ not a section
/* }%% and not the end of one */
var a = "%%{ in a string }%%";
var b = '}';
var c = `template ${a.length} }`;
var re = /[a-z]+/;

%%{
	machine js2;

	action nl {
		// a } in a comment
		/* another } */
		var s = "}";
		var t = '{';
		var u = `${s}${t}`;
		console.log( "NL " + u );
	}

	main := lower+ digit+ '\n' @nl;
}%%

// %%{
var d = "}%%";

%% write data;

/* %% write init; is not a write statement here */
var e = "%% write exec;";

function run( data )
{
	var p = 0;
	var pe = data.length;
	var cs = 0;

	%% write init;
	%% write exec;

	if ( cs >= js2_first_final )
		console.log( "ACCEPT" );
	else
		console.log( "FAIL" );
}

run( "abc1231\n" );
run( "abc\n" );

##### OUTPUT #####
NL }{
ACCEPT
FAIL
