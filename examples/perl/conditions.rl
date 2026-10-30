use strict;
use warnings;
%%{
 machine conditions;
 action flag_one { $one }
 action flag_two { $two }
 action seen_one { $trace .= '1'; }
 action seen_two { $trace .= '2'; }
 main := ('a' 'b' when flag_one @seen_one | 'a' 'b' when flag_two @seen_two);
}%%
%% write data;
for my $flags ([0,0], [1,0], [0,1], [1,1]) {
 my ($one, $two) = @$flags;
 my $data = 'ab';
 my ($p, $pe, $cs) = (0, length($data), 0);
 my $trace = '';
 %% write init;
 %% write exec;
 print "$trace:", ($cs >= $conditions_first_final ? 'ACCEPT' : 'FAIL'), "\n";
}
