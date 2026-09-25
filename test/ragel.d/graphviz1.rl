%%{
	#
	# @LANG: graphviz
	#
	# Every host binary writes the dot for -V to standard out. They once
	# handed it to the host code translator as the intermediate file, which
	# rejected it.
	#

	machine graphviz1;

	action done {}

	main := ( 'foo' | 'bar' ) 0 @done;
}%%

##### OUTPUT #####
digraph graphviz1 {
	rankdir=LR;
	node [ shape = point ];
	ENTRY;
	en_1;
	node [ shape = circle, height = 0.2 ];
	node [ fixedsize = true, height = 0.65, shape = doublecircle ];
	7;
	node [ shape = circle ];
	1 -> 2 [ label = "98" ];
	1 -> 5 [ label = "102" ];
	2 -> 3 [ label = "97" ];
	3 -> 4 [ label = "114" ];
	4 -> 7 [ label = "0 / done" ];
	5 -> 6 [ label = "111" ];
	6 -> 4 [ label = "111" ];
	ENTRY -> 1 [ label = "IN" ];
	en_1 -> 1 [ label = "main" ];
}
