%%{
	#
	# @LANG: error
	#
	# A bad variable name is reported with just the name.
	#

	machine varname1;

	variable bogus x;

	main := "a";
}%%

##### OUTPUT #####
10:2: bad variable name: bogus
