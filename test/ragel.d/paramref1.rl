%%{
	#
	# @LANG: error
	#
	# A reference to a parameter an action doesn't have is reported with
	# the name as written, one $ in front.
	#

	machine paramref1;

	action c1 {}
	action c(v) { $w }

	main := "a" >c(c1);
}%%

##### OUTPUT #####
12:16: invalid parameter reference "$w"
