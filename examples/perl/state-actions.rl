use strict;
use warnings;
use bytes;
%%{
 machine state_actions;
 action enter { $trace .= 'T'; }
 action leave { $trace .= 'F'; }
 action transition { $trace .= 'X'; }
 action ended { $trace .= 'E'; }
 main := ('ab' $to enter $from leave $transition) %eof ended;
}%%
%% write data;
for my $data ('ab', 'a', 'x', '') {
 my ($p, $pe, $cs) = (0, length($data), 0);
 my $eof = $pe;
 my $trace = '';
 %% write init;
 %% write exec;
 print "$trace:", ($cs >= $state_actions_first_final ? 'ACCEPT' : 'FAIL'), "\n";
}
