use strict;
use warnings;
use bytes;

%%{
    machine streaming_greeting;
    action add_name { $name .= chr(fc); }
    main := 'HELLO:' ([A-Z]+ $add_name) '\n';
}%%
%% write data;

my $cs;
my $name = '';
my $chunk = 0;
%% write init;
for my $data ('HEL', 'LO:AL', "ICE\n") {
    my ($p, $pe) = (0, length($data));
    %% write exec;
    die "Invalid chunk\n" if $cs == $streaming_greeting_error || $p != $pe;
    print "chunk ", ++$chunk, ": name=$name\n";
}
print $cs >= $streaming_greeting_first_final ? "ACCEPT: $name\n" : "INCOMPLETE\n";
