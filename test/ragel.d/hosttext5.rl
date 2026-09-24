/*
 * @LANG: c
 *
 * Host text with the embedded code openers and closers of the intermediate
 * language: ={ }= ${ }$ }@ and @{, in the host sections and inside an action
 * block, where the text arrives one token at a time.
 */
#include <stdio.h>

struct s { int v; };

%%{
	machine hosttext5;

	action a { struct s x ={ 1 }; printf( "%d ${ }$ ={ }= }@ @{\n", x.v ); }

	main := 'a' @a;
}%%

int arr[] ={ 2 };

%% write data;

int main()
{
	int cs;
	const char *p = "a", *pe = p + 1;

	%% write init;
	%% write exec;

	printf( "%d ${ }$ ={ }= }@ @{\n", arr[0] );
	return 0;
}

##### OUTPUT #####
1 ${ }$ ={ }= }@ @{
2 ${ }$ ={ }= }@ @{
