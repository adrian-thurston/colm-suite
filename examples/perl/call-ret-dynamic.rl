use strict;
use warnings;
%%{
 machine call_ret;
 action call_sub { $trace .= 'C'; fcall *fentry(submachine); $trace .= 'X'; }
 action return_sub { $trace .= 'R'; fret; $trace .= 'X'; }
 main := 'a' @call_sub 'c';
 submachine := 'b' @return_sub;
}%%
%% write data;
my $data = 'abc';
my ($p, $pe, $cs, $top) = (0, length($data), 0, 0);
my @stack;
my $trace = '';
%% write init;
%% write exec;
print "$trace:$p:", ($cs >= $call_ret_first_final ? 'ACCEPT' : 'FAIL'), "\n";
