%%{
	#
	# @LANG: error
	#
	# An NFA round's group too large for a long is reported at the group,
	# with just its digits.
	#

	machine nfaround1;

	main |= ( 5, 99999999999999999999999 ) ( "a" | "b" );
}%%

##### OUTPUT #####
11:15: group 99999999999999999999999 overflows
