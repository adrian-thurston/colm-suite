use strict;
use warnings;
use bytes;

%%{
    machine setting;
    action add_key   { $key .= chr(fc); }
    action add_value { $value .= chr(fc); }
    key   = [a-z]+ $add_key;
    value = [a-z]+ $add_value;
    main := key '=' value;
}%%
%% write data;

for my $data ('color=purple', 'mode=fast', 'bad=', '=missing') {
    my ($p, $pe, $cs) = (0, length($data), 0);
    my ($key, $value) = ('', '');
    %% write init;
    %% write exec;
    if ($cs >= $setting_first_final && $p == $pe) {
        print "$data: key=$key value=$value\n";
    }
    else {
        print "$data: INVALID\n";
    }
}
