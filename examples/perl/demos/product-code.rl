use strict;
use warnings;
use bytes;

%%{
    machine product_code;
    main := [A-Z]{3} '-' [0-9]{4};
}%%
%% write data;

for my $data ('ABC-1234', 'XYZ-9000', 'abc-1234', 'ABC-123') {
    my ($p, $pe, $cs) = (0, length($data), 0);
    %% write init;
    %% write exec;
    my $valid = $cs >= $product_code_first_final && $p == $pe;
    print "$data: ", ($valid ? 'VALID' : 'INVALID'), "\n";
}
