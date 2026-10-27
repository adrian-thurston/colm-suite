%%{
	#
	# @LANG: error
	#
	# A repetition number too large for an int is an overflow. It was
	# truncated, here to 1.
	#

	machine repnum1;

	main := "a"{4294967297};
}%%

##### OUTPUT #####
11:14: repetition number 4294967297 overflows
