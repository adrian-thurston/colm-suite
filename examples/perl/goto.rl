use strict;
use warnings;
%%{
 machine jump;
 action jump_now { $trace .= 'J'; fgoto other; $trace .= 'X'; }
 main := 'a' @jump_now;
 other := 'b';
}%%
%% write data;
my $data = 'ab';
my ($p, $pe, $cs) = (0, length($data), 0);
my $trace = '';
%% write init;
%% write exec;
print "$trace:$p:", ($cs >= $jump_first_final ? 'ACCEPT' : 'FAIL'), "\n";
