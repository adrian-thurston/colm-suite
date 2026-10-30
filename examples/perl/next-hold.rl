use strict;
use warnings;
%%{
 machine next_hold;
 action jump { fhold; fnext other; }
 main := 'a' @jump;
 other := 'a';
}%%
%% write data;
my $data = 'a';
my ($p, $pe, $cs) = (0, length($data), 0);
%% write init;
%% write exec;
print "position=$p:", ($cs >= $next_hold_first_final ? 'ACCEPT' : 'FAIL'), "\n";
