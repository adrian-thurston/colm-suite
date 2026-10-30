use strict;
use warnings;
use bytes;

sub record_user_byte {
    my ($event, $byte) = @_;
    $event->{user} .= chr($byte);
}

sub record_result_byte {
    my ($event, $byte) = @_;
    $event->{result} .= chr($byte);
}

%%{
    machine login_event;
    action user_byte   { record_user_byte($event, fc); }
    action result_byte { record_result_byte($event, fc); }
    username = [a-z0-9_]+ $user_byte;
    result   = ('OK' | 'FAIL') $result_byte;
    main := 'LOGIN user=' username ' result=' result;
}%%
%% write data;

for my $data ('LOGIN user=alice result=OK',
              'LOGIN user=carol result=FAIL',
              'LOGIN user=bad-name result=OK') {
    my $event = { user => '', result => '' };
    my ($p, $pe, $cs) = (0, length($data), 0);
    %% write init;
    %% write exec;
    if ($cs >= $login_event_first_final && $p == $pe) {
        print "user=$event->{user} result=$event->{result}\n";
    }
    else {
        print "INVALID: $data\n";
    }
}
