%%{
	#
	# @LANG: error
	#
	# A priority too large for an int is an overflow. It was truncated, here
	# to 1.
	#

	machine priornum1;

	main := ( "a" > 4294967297 );
}%%

##### OUTPUT #####
11:18: priority number 4294967297 overflows
