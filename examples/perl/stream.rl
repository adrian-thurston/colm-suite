use strict;
use warnings;
use bytes;
%%{
    machine stream;
    action record { $seen .= chr(fc); }
    main := 'HELLO:' ([A-Z]+ $record) '\n';
}%%
%% write data;
my $cs;
my $seen = '';
%% write init;
for my $data ('HEL', 'LO:AL', "ICE\n") {
    my $p = 0;
    my $pe = length $data;
    %% write exec;
    die "chunk not consumed" if $p != $pe;
}
print(($cs >= $stream_first_final ? 'ACCEPT' : 'FAIL'), ":$seen\n");
