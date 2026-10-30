
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



my $event_ping_start = 1;
my $event_ping_first_final = 9;
my $event_ping_error = 0;
my @_event_ping_offsets = (0,0,3,6,9,12,15,18,23,26,27,);
my @_event_ping_low = (0,80,81,0,73,74,0,78,79,0,71,72,0,32,33,0,65,91,0,13,14,65,91,0,10,11,0,);
my @_event_ping_high = (79,80,255,72,73,255,77,78,255,70,71,255,31,32,255,64,90,255,12,13,64,90,255,9,10,255,255,);
my @_event_ping_targets = (0,2,0,0,3,0,0,4,0,0,5,0,0,6,0,0,7,0,0,8,0,7,0,0,9,0,0,);
my @_event_ping_actions = (0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,);


socketpair(my $server_fh, my $client_fh, AF_UNIX, SOCK_STREAM, PF_UNSPEC)
    or die "socketpair: $!";

my $loop = Linux::Event::Loop->new;
my ($cs, $name, $completed, $received) = (0, '', 0, 0);

$cs = 1;


my $stream = Linux::Event::IO::Sock::Stream->new(
    loop => $loop,
    fh => $server_fh,
    on_data => sub {
        my ($connection, $data) = @_;
        $received += length($data);
        my ($p, $pe) = (0, length($data));

{
 use bytes;
 while ($p < $pe && $cs != 0) {
  my $_ragel_key = ord(substr($data, $p, 1));
  my $_ragel_trans = -1;
  if ($cs >= 0 && $cs < 10) {
   my $_ragel_lo = $_event_ping_offsets[$cs];
   my $_ragel_hi = $_event_ping_offsets[$cs + 1] - 1;
   while ($_ragel_lo <= $_ragel_hi) {
    my $_ragel_mid = int(($_ragel_lo + $_ragel_hi) / 2);
    if ($_ragel_key < $_event_ping_low[$_ragel_mid]) { $_ragel_hi = $_ragel_mid - 1; }
    elsif ($_ragel_key > $_event_ping_high[$_ragel_mid]) { $_ragel_lo = $_ragel_mid + 1; }
    else { $_ragel_trans = $_ragel_mid; last; }
   }
  }
  my $_ragel_action = 0;
  if ($_ragel_trans >= 0) {
   $cs = $_event_ping_targets[$_ragel_trans];
   $_ragel_action = $_event_ping_actions[$_ragel_trans];
  } else { $cs = 0; }
  if ($_ragel_action == 1) {
 $name .= chr(ord(substr($data, $p, 1)));
  }
  last if $cs == 0;
  ++$p;
 }
 RAGEL_PERL_OUT: ;
}

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
