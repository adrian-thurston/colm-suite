
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



my $demo_http_start = 1;
my $demo_http_first_final = 22;
my $demo_http_error = 0;
my @_demo_http_offsets = (0,0,3,6,9,12,15,28,31,34,37,40,43,46,49,52,55,58,69,72,82,90,95,96,);
my @_demo_http_low = (0,71,72,0,69,70,0,84,85,0,32,33,0,47,48,0,32,33,45,46,47,58,65,91,95,96,97,123,0,72,73,0,84,85,0,84,85,0,80,81,0,47,48,0,49,50,0,46,47,0,49,50,0,13,14,0,10,11,0,13,14,45,46,48,58,65,91,97,123,0,10,11,0,45,46,48,58,59,65,91,97,123,0,9,10,13,14,32,33,127,0,13,14,32,127,0,);
my @_demo_http_high = (70,71,255,68,69,255,83,84,255,31,32,255,46,47,255,31,32,44,45,46,57,64,90,94,95,96,122,255,71,72,255,83,84,255,83,84,255,79,80,255,46,47,255,48,49,255,45,46,255,48,49,255,12,13,255,9,10,255,12,13,44,45,47,57,64,90,96,122,255,9,10,255,44,45,47,57,58,64,90,96,122,255,8,9,12,13,31,32,126,255,12,13,31,126,255,255,);
my @_demo_http_targets = (0,2,0,0,3,0,0,4,0,0,5,0,0,6,0,0,7,0,6,0,6,0,6,0,6,0,6,0,0,8,0,0,9,0,0,10,0,0,11,0,0,12,0,0,13,0,0,14,0,0,15,0,0,16,0,0,17,0,0,18,0,19,0,19,0,19,0,19,0,0,22,0,0,19,0,19,20,0,19,0,19,0,0,20,0,16,0,20,21,0,0,16,0,21,0,0,);
my @_demo_http_actions = (0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,);


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

$cs = 1;


    my $stream = Linux::Event::IO::Sock::Stream->new(
        loop => $loop,
        fh => $server_fh,
        on_data => sub {
            my ($connection, $data) = @_;
            return if $completed;
            $received += length($data);
            my ($p, $pe) = (0, length($data));

{
 use bytes;
 while ($p < $pe && $cs != 0) {
  my $_ragel_key = ord(substr($data, $p, 1));
  my $_ragel_trans = -1;
  if ($cs >= 0 && $cs < 23) {
   my $_ragel_lo = $_demo_http_offsets[$cs];
   my $_ragel_hi = $_demo_http_offsets[$cs + 1] - 1;
   while ($_ragel_lo <= $_ragel_hi) {
    my $_ragel_mid = int(($_ragel_lo + $_ragel_hi) / 2);
    if ($_ragel_key < $_demo_http_low[$_ragel_mid]) { $_ragel_hi = $_ragel_mid - 1; }
    elsif ($_ragel_key > $_demo_http_high[$_ragel_mid]) { $_ragel_lo = $_ragel_mid + 1; }
    else { $_ragel_trans = $_ragel_mid; last; }
   }
  }
  my $_ragel_action = 0;
  if ($_ragel_trans >= 0) {
   $cs = $_demo_http_targets[$_ragel_trans];
   $_ragel_action = $_demo_http_actions[$_ragel_trans];
  } else { $cs = 0; }
  if ($_ragel_action == 1) {
 $path .= chr(ord(substr($data, $p, 1)));
  }
  last if $cs == 0;
  ++$p;
 }
 RAGEL_PERL_OUT: ;
}

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
