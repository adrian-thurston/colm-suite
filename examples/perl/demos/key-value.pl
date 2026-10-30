
use strict;
use warnings;
use bytes;




my $setting_start = 1;
my $setting_first_final = 4;
my $setting_error = 0;
my @_setting_offsets = (0,0,3,8,11,14,);
my @_setting_low = (0,97,123,0,61,62,97,123,0,97,123,0,97,123,);
my @_setting_high = (96,122,255,60,61,96,122,255,96,122,255,96,122,255,);
my @_setting_targets = (0,2,0,0,3,0,2,0,0,4,0,0,4,0,);
my @_setting_actions = (0,1,0,0,0,0,1,0,0,2,0,0,2,0,);


for my $data ('color=purple', 'mode=fast', 'bad=', '=missing') {
    my ($p, $pe, $cs) = (0, length($data), 0);
    my ($key, $value) = ('', '');

$cs = 1;


{
 use bytes;
 while ($p < $pe && $cs != 0) {
  my $_ragel_key = ord(substr($data, $p, 1));
  my $_ragel_trans = -1;
  if ($cs >= 0 && $cs < 5) {
   my $_ragel_lo = $_setting_offsets[$cs];
   my $_ragel_hi = $_setting_offsets[$cs + 1] - 1;
   while ($_ragel_lo <= $_ragel_hi) {
    my $_ragel_mid = int(($_ragel_lo + $_ragel_hi) / 2);
    if ($_ragel_key < $_setting_low[$_ragel_mid]) { $_ragel_hi = $_ragel_mid - 1; }
    elsif ($_ragel_key > $_setting_high[$_ragel_mid]) { $_ragel_lo = $_ragel_mid + 1; }
    else { $_ragel_trans = $_ragel_mid; last; }
   }
  }
  my $_ragel_action = 0;
  if ($_ragel_trans >= 0) {
   $cs = $_setting_targets[$_ragel_trans];
   $_ragel_action = $_setting_actions[$_ragel_trans];
  } else { $cs = 0; }
  if ($_ragel_action == 1) {
 $key .= chr(ord(substr($data, $p, 1)));
  }
  if ($_ragel_action == 2) {
 $value .= chr(ord(substr($data, $p, 1)));
  }
  last if $cs == 0;
  ++$p;
 }
 RAGEL_PERL_OUT: ;
}

    if ($cs >= $setting_first_final && $p == $pe) {
        print "$data: key=$key value=$value\n";
    }
    else {
        print "$data: INVALID\n";
    }
}
