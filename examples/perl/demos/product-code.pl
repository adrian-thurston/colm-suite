
use strict;
use warnings;
use bytes;




my $product_code_start = 1;
my $product_code_first_final = 9;
my $product_code_error = 0;
my @_product_code_offsets = (0,0,3,6,9,12,15,18,21,24,25,);
my @_product_code_low = (0,65,91,0,65,91,0,65,91,0,45,46,0,48,58,0,48,58,0,48,58,0,48,58,0,);
my @_product_code_high = (64,90,255,64,90,255,64,90,255,44,45,255,47,57,255,47,57,255,47,57,255,47,57,255,255,);
my @_product_code_targets = (0,2,0,0,3,0,0,4,0,0,5,0,0,6,0,0,7,0,0,8,0,0,9,0,0,);
my @_product_code_actions = (0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,);


for my $data ('ABC-1234', 'XYZ-9000', 'abc-1234', 'ABC-123') {
    my ($p, $pe, $cs) = (0, length($data), 0);

$cs = 1;


{
 use bytes;
 while ($p < $pe && $cs != 0) {
  my $_ragel_key = ord(substr($data, $p, 1));
  my $_ragel_trans = -1;
  if ($cs >= 0 && $cs < 10) {
   my $_ragel_lo = $_product_code_offsets[$cs];
   my $_ragel_hi = $_product_code_offsets[$cs + 1] - 1;
   while ($_ragel_lo <= $_ragel_hi) {
    my $_ragel_mid = int(($_ragel_lo + $_ragel_hi) / 2);
    if ($_ragel_key < $_product_code_low[$_ragel_mid]) { $_ragel_hi = $_ragel_mid - 1; }
    elsif ($_ragel_key > $_product_code_high[$_ragel_mid]) { $_ragel_lo = $_ragel_mid + 1; }
    else { $_ragel_trans = $_ragel_mid; last; }
   }
  }
  my $_ragel_action = 0;
  if ($_ragel_trans >= 0) {
   $cs = $_product_code_targets[$_ragel_trans];
   $_ragel_action = $_product_code_actions[$_ragel_trans];
  } else { $cs = 0; }
  last if $cs == 0;
  ++$p;
 }
 RAGEL_PERL_OUT: ;
}

    my $valid = $cs >= $product_code_first_final && $p == $pe;
    print "$data: ", ($valid ? 'VALID' : 'INVALID'), "\n";
}
