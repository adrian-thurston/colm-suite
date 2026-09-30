/*
 * @LANG: js
 *
 * Host text with regular expression and template literals that hold the
 * section delimiters, braces and quotes, which the frontend once took for code
 * and the js output grammar for strings and blocks. A slash after an operand
 * is division and opens no literal, so an fc after it is still translated.
 */

var r1 = /%%/, r2 = /"/, r3 = /}/, r4 = /'/, r5 = /[/]/g;
var t1 = `%%`, t2 = `}%%`;
var h = 12 / 6, g = "/";
var w = ( h ) / 2 + [ 8 ][ 0 ] / h // }
	/ 1;
function f( s ) { return /}/.test( s ); }
var k = s => !/{/.test( s ) ? /'/ : /"/;

%%{
	machine hosttext6;

	action a {
		var r6 = /\}/, r7 = /\{/, r8 = /"/, r9 = /[}]/;
		var t3 = `}`;
		var n = ( fc - 48 ) / 2, m = fc / 2 - fc / 2 + n // }
			/ 2;
		if ( n ) { n /= m; }
		console.log( r6.test( t3 ), r7.source, r8.source, r9.test( "}" ), n, m );
	}

	main := digit @a;
}%%

%% write data;

function run( data )
{
	var p = 0;
	var pe = data.length;
	var cs = 0;

	%% write init;
	%% write exec;

	if ( cs >= hosttext6_first_final )
		console.log( "ACCEPT" );
	else
		console.log( "FAIL" );
}

run( "4" );
console.log( r1.test( "%%" ), r2.source, r3.source, r4.source, "a/b".replace( r5, "-" ) );
console.log( h, t1, t2, g, w, f( "}" ), k( "a" ).source, k( "{" ).source );

##### OUTPUT #####
true \{ " true 2 1
ACCEPT
true " } ' a-b
2 %% }%% / 5 true ' "
