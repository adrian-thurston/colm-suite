use v5.36;
use strict;
use warnings;
use bytes;
use Socket qw(AF_UNIX SOCK_STREAM PF_UNSPEC);
use IO::Select;
use Linux::Event 0.117;
use Linux::Event::Loop;
use Linux::Event::IO::Sock::Stream;

# One request per connection: PING followed by a name and CRLF.
%%{
    machine event_ping;
    action collect { $name .= chr(fc); }
    main := 'PING ' ([A-Z]+ $collect) '\r\n';
}%%
%% write data;

socketpair(my $server_fh, my $client_fh, AF_UNIX, SOCK_STREAM, PF_UNSPEC)
    or die "socketpair: $!";

my $loop = Linux::Event::Loop->new;
my ($cs, $name, $completed, $received) = (0, '', 0, 0);
%% write init;

my $stream = Linux::Event::IO::Sock::Stream->new(
    loop => $loop,
    fh => $server_fh,
    on_data => sub {
        my ($connection, $data) = @_;
        $received += length($data);
        my ($p, $pe) = (0, length($data));
        %% write exec;
        die "Invalid request\n" if $cs == $event_ping_error || $p != $pe;
        if ($cs >= $event_ping_first_final) {
            die "Repeated request\n" if $completed;
            $completed = 1;
            $connection->write("PONG $name\r\n");
        }
    },
    on_error => sub { die "Stream error: $_[1]\n" },
);

# Force separate event-loop turns to exercise the parser's retained state.
syswrite($client_fh, 'PING ALI') == 8 or die "first write: $!";
$loop->run_for(0.05);
die "First chunk was not delivered\n" unless $received == 8 && !$completed;
syswrite($client_fh, "CE\r\n") == 4 or die "second write: $!";
$loop->run_for(0.05);
die "Request incomplete\n" unless $completed;

my $select = IO::Select->new($client_fh);
my $reply = '';
while (length($reply) < length("PONG ALICE\r\n")) {
    die "Timed out waiting for reply\n" unless $select->can_read(1);
    my $n = sysread($client_fh, my $part, 1024);
    die "Reply read failed: $!" unless defined $n;
    die "Unexpected EOF\n" if $n == 0;
    $reply .= $part;
}
die "Wrong reply\n" unless $reply eq "PONG ALICE\r\n";
print "Request parsed: PING $name\n";
print "Response received: PONG $name\n";
