use strict;
use warnings;

%%{
    machine greeting;
    action begin_name { $name .= '['; }
    action remember { $name .= chr(fc); }
    main := 'HELLO:' ([A-Z]+ >begin_name $remember) '\n';
}%%

%% write data;

for my $data ("HELLO:ALICE\n", "HELLO:\n", "HELLO:alice\n") {
    my ($p, $pe, $cs) = (0, length($data), 0);
    my $name = '';
    %% write init;
    %% write exec;
    print(($cs >= $greeting_first_final ? 'ACCEPT' : 'FAIL'), ":$name\n");
}
