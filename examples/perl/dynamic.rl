use strict;
use warnings;
%%{
 machine dynamic_jump;
 action jump { fexec 0; fnext *fentry(other); }
 main := 'a' @jump;
 other := 'a';
}%%
%% write data;
my $data = 'a';
my ($p, $pe, $cs) = (0, length($data), 0);
%% write init;
%% write exec;
print "position=$p:", ($cs >= $dynamic_jump_first_final ? 'ACCEPT' : 'FAIL'), "\n";
