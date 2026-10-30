
use strict;
use warnings;





my $greeting_start = 1;
my $greeting_first_final = 9;
my $greeting_error = 0;
my @_greeting_offsets = (0,0,3,6,9,12,15,18,21,26,27,);
my @_greeting_low = (0,72,73,0,69,70,0,76,77,0,76,77,0,79,80,0,58,59,0,65,91,0,10,11,65,91,0,);
my @_greeting_high = (71,72,255,68,69,255,75,76,255,75,76,255,78,79,255,57,58,255,64,90,255,9,10,64,90,255,255,);
my @_greeting_targets = (0,2,0,0,3,0,0,4,0,0,5,0,0,6,0,0,7,0,0,8,0,0,9,0,8,0,0,);
my @_greeting_actions = (0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,2,0,0,);


for my $data ("HELLO:ALICE\n", "HELLO:\n", "HELLO:alice\n") {
    my ($p, $pe, $cs) = (0, length($data), 0);
    my $name = '';

$cs = 1;


{
 use bytes;
 while ($p < $pe && $cs != 0) {
  my $_ragel_key = ord(substr($data, $p, 1));
  my $_ragel_trans = -1;
  if ($cs >= 0 && $cs < 10) {
   my $_ragel_lo = $_greeting_offsets[$cs];
   my $_ragel_hi = $_greeting_offsets[$cs + 1] - 1;
   while ($_ragel_lo <= $_ragel_hi) {
    my $_ragel_mid = int(($_ragel_lo + $_ragel_hi) / 2);
    if ($_ragel_key < $_greeting_low[$_ragel_mid]) { $_ragel_hi = $_ragel_mid - 1; }
    elsif ($_ragel_key > $_greeting_high[$_ragel_mid]) { $_ragel_lo = $_ragel_mid + 1; }
    else { $_ragel_trans = $_ragel_mid; last; }
   }
  }
  my $_ragel_action = 0;
  if ($_ragel_trans >= 0) {
   $cs = $_greeting_targets[$_ragel_trans];
   $_ragel_action = $_greeting_actions[$_ragel_trans];
  } else { $cs = 0; }
  if ($_ragel_action == 1) {
 $name .= '[';
 $name .= chr(ord(substr($data, $p, 1)));
  }
  if ($_ragel_action == 2) {
 $name .= chr(ord(substr($data, $p, 1)));
  }
  last if $cs == 0;
  ++$p;
 }
 RAGEL_PERL_OUT: ;
}

    print(($cs >= $greeting_first_final ? 'ACCEPT' : 'FAIL'), ":$name\n");
}
