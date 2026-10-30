use strict;
use warnings;
%%{
 machine control;
 action stop { fbreak; }
 main := 'abc' @stop 'd';
}%%
%% write data;
my $data = 'abcd';
my ($p, $pe, $cs) = (0, length($data), 0);
%% write init;
%% write exec noend;
print "position=$p\n";
