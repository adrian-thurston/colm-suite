%%{
	#
	# @LANG: error
	#
	# An NFA round's depth too large for a long is reported with just its
	# digits.
	#

	machine nfaround2;

	main |= ( 99999999999999999999999, 1 ) ( "a" | "b" );
}%%

##### OUTPUT #####
11:12: rounds 99999999999999999999999 overflows
