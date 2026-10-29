%%{
	#
	# @LANG: error
	#
	# An NFA round's depth too large for an int is an overflow. It was
	# truncated, here to depth 0.
	#

	machine nfaround4;

	main |= ( 4294967296, 1 ) ( "a" | "b" );
}%%

##### OUTPUT #####
11:12: rounds 4294967296 overflows
