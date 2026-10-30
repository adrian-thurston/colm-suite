
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




my $login_event_start = 1;
my $login_event_first_final = 26;
my $login_event_error = 0;
my @_login_event_offsets = (0,0,3,6,9,12,15,18,21,24,27,30,33,40,49,52,55,58,61,64,67,70,75,78,81,84,87,88,);
my @_login_event_low = (0,76,77,0,79,80,0,71,72,0,73,74,0,78,79,0,32,33,0,117,118,0,115,116,0,101,102,0,114,115,0,61,62,0,48,58,95,96,97,123,0,32,33,48,58,95,96,97,123,0,114,115,0,101,102,0,115,116,0,117,118,0,108,109,0,116,117,0,61,62,0,70,71,79,80,0,65,66,0,73,74,0,76,77,0,75,76,0,);
my @_login_event_high = (75,76,255,78,79,255,70,71,255,72,73,255,77,78,255,31,32,255,116,117,255,114,115,255,100,101,255,113,114,255,60,61,255,47,57,94,95,96,122,255,31,32,47,57,94,95,96,122,255,113,114,255,100,101,255,114,115,255,116,117,255,107,108,255,115,116,255,60,61,255,69,70,78,79,255,64,65,255,72,73,255,75,76,255,74,75,255,255,);
my @_login_event_targets = (0,2,0,0,3,0,0,4,0,0,5,0,0,6,0,0,7,0,0,8,0,0,9,0,0,10,0,0,11,0,0,12,0,0,13,0,13,0,13,0,0,14,0,13,0,13,0,13,0,0,15,0,0,16,0,0,17,0,0,18,0,0,19,0,0,20,0,0,21,0,0,22,0,25,0,0,23,0,0,24,0,0,26,0,0,26,0,0,);
my @_login_event_actions = (0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,2,0,0,2,0,0,2,0,0,2,0,0,2,0,0,);


for my $data ('LOGIN user=alice result=OK',
              'LOGIN user=carol result=FAIL',
              'LOGIN user=bad-name result=OK') {
    my $event = { user => '', result => '' };
    my ($p, $pe, $cs) = (0, length($data), 0);

$cs = 1;


{
 use bytes;
 while ($p < $pe && $cs != 0) {
  my $_ragel_key = ord(substr($data, $p, 1));
  my $_ragel_trans = -1;
  if ($cs >= 0 && $cs < 27) {
   my $_ragel_lo = $_login_event_offsets[$cs];
   my $_ragel_hi = $_login_event_offsets[$cs + 1] - 1;
   while ($_ragel_lo <= $_ragel_hi) {
    my $_ragel_mid = int(($_ragel_lo + $_ragel_hi) / 2);
    if ($_ragel_key < $_login_event_low[$_ragel_mid]) { $_ragel_hi = $_ragel_mid - 1; }
    elsif ($_ragel_key > $_login_event_high[$_ragel_mid]) { $_ragel_lo = $_ragel_mid + 1; }
    else { $_ragel_trans = $_ragel_mid; last; }
   }
  }
  my $_ragel_action = 0;
  if ($_ragel_trans >= 0) {
   $cs = $_login_event_targets[$_ragel_trans];
   $_ragel_action = $_login_event_actions[$_ragel_trans];
  } else { $cs = 0; }
  if ($_ragel_action == 1) {
 record_user_byte($event, ord(substr($data, $p, 1)));
  }
  if ($_ragel_action == 2) {
 record_result_byte($event, ord(substr($data, $p, 1)));
  }
  last if $cs == 0;
  ++$p;
 }
 RAGEL_PERL_OUT: ;
}

    if ($cs >= $login_event_first_final && $p == $pe) {
        print "user=$event->{user} result=$event->{result}\n";
    }
    else {
        print "INVALID: $data\n";
    }
}
