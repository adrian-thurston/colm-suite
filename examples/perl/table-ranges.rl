use strict;
use warnings;
%%{
 machine table_ranges;
 action letter { $trace .= 'L'; }
 action digit { $trace .= 'D'; }
 main := (([a-z] @letter | [0-9] @digit) [A-F]?);
}%%
%% write data;
for my $first (0..127) {
 for my $second (-1..127) {
  my $data = chr($first) . ($second < 0 ? '' : chr($second));
  my ($p, $pe, $cs) = (0, length($data), 0);
  my $trace = '';
  %% write init;
  %% write exec;
  print "$first,$second:", ($cs >= $table_ranges_first_final ? 1 : 0), ":$p:$trace\n";
 }
}
