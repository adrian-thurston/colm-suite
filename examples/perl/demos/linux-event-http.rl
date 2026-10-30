use v5.36;
use strict;
use warnings;
use bytes;
use Socket qw(AF_UNIX SOCK_STREAM PF_UNSPEC);
use IO::Select;
use Linux::Event 0.117;
use Linux::Event::Loop;
use Linux::Event::IO::Sock::Stream;

# Educational subset: GET, HTTP/1.1, ASCII headers, one request per socket.
%%{
    machine demo_http;
    action path_byte { $path .= chr(fc); }
    target = ('/' [A-Za-z0-9/_\-]*) $path_byte;
    header = [A-Za-z0-9\-]+ ':' [ \t]* (0x20..0x7e)* '\r\n';
    main := 'GET ' target ' HTTP/1.1\r\n' header* '\r\n';
}%%
%% write data;

sub response_for {
    my ($status, $body) = @_;
    return "HTTP/1.1 $status\r\n"
        . "Content-Type: text/plain\r\n"
        . 'Content-Length: ' . length($body) . "\r\n"
        . "Connection: close\r\n\r\n"
        . $body;
}

sub run_case {
    my ($label, $chunks, $expected_status, $expected_body) = @_;
    socketpair(my $server_fh, my $client_fh, AF_UNIX, SOCK_STREAM, PF_UNSPEC)
        or die "socketpair: $!";
    my $loop = Linux::Event::Loop->new;
    my ($cs, $path, $completed, $received) = (0, '', 0, 0);
    %% write init;

    my $stream = Linux::Event::IO::Sock::Stream->new(
        loop => $loop,
        fh => $server_fh,
        on_data => sub {
            my ($connection, $data) = @_;
            return if $completed;
            $received += length($data);
            my ($p, $pe) = (0, length($data));
            %% write exec;
            my ($status, $body);
            if ($received > 4096 || $cs == $demo_http_error || $p != $pe) {
                ($status, $body) = ('400 Bad Request', "Bad request\n");
            }
            elsif ($cs >= $demo_http_first_final) {
                if ($path eq '/health') {
                    ($status, $body) = ('200 OK', "healthy\n");
                }
                elsif ($path =~ m{^/hello/([A-Za-z0-9_-]+)$}) {
                    ($status, $body) = ('200 OK', "Hello, $1!\n");
                }
                else {
                    ($status, $body) = ('404 Not Found', "Not found\n");
                }
            }
            if (defined $status) {
                $completed = 1;
                $connection->write(response_for($status, $body));
            }
        },
        on_error => sub { die "Stream error: $_[1]\n" },
    );

    my $sent = 0;
    for my $chunk (@$chunks) {
        my $n = syswrite($client_fh, $chunk);
        die "write failed or was short: $!" unless defined($n) && $n == length($chunk);
        $sent += $n;
        $loop->run_for(0.05);
        die "Input not delivered\n" unless $received == $sent;
    }
    die "No response for $label\n" unless $completed;
    $loop->run_for(0.05);

    my $expected = response_for($expected_status, $expected_body);
    my $reply = '';
    my $select = IO::Select->new($client_fh);
    while (length($reply) < length($expected)) {
        die "Timed out waiting for $label\n" unless $select->can_read(1);
        my $n = sysread($client_fh, my $part, 1024);
        die "read failed: $!" unless defined $n;
        die "Unexpected EOF\n" if $n == 0;
        $reply .= $part;
    }
    die "Unexpected response for $label\n" unless $reply eq $expected;
    print "$label: $expected_status; body=$expected_body";
}

run_case('fragmented hello', [ 'GET /hel', "lo/ALICE HTTP/1.1\r\nHost: local\r\n", "\r\n" ], '200 OK', "Hello, ALICE!\n");
run_case('health', [ "GET /health HTTP/1.1\r\nHost: local\r\n\r\n" ], '200 OK', "healthy\n");
run_case('unknown path', [ "GET /missing HTTP/1.1\r\n\r\n" ], '404 Not Found', "Not found\n");
run_case('unsupported method', [ "POST /health HTTP/1.1\r\n\r\n" ], '400 Bad Request', "Bad request\n");
