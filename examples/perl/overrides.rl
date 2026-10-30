use strict;
use warnings;
%%{
 machine overrides;
 variable p $pos;
 variable pe $end;
 variable eof $end_of_input;
 variable cs $state;
 variable data $input;
 action seen { $trace .= chr(fc); }
 action ended { $trace .= '!'; }
 main := ('ab' $seen) %eof ended;
}%%
%% write data;
my $input = 'ab';
my ($pos, $end, $state) = (0, length($input), 0);
my $end_of_input = $end;
my $trace = '';
%% write init;
%% write exec;
print "$trace:$pos:", ($state >= $overrides_first_final ? 'ACCEPT' : 'FAIL'), "\n";
