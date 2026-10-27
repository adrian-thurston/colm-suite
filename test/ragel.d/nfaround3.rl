%%{
	#
	# @LANG: error
	#
	# An NFA round's group too large for an int is an overflow. It was
	# truncated to a negative group size, and ragel crashed.
	#

	machine nfaround3;

	main |= ( 5, 2147483648 ) ( "a" | "b" );
}%%

##### OUTPUT #####
11:15: group 2147483648 overflows
