%%{
 machine table_ranges;
 action letter { trace << 'L' }
 action digit { trace << 'D' }
 main := (([a-z] @letter | [0-9] @digit) [A-F]?);
}%%
%% write data;
(0..127).each do |first|
 (-1..127).each do |second|
  data = first.chr + (second < 0 ? '' : second.chr)
  p = 0
  pe = data.length
  cs = 0
  trace = ''
  %% write init;
  %% write exec;
  puts "#{first},#{second}:#{cs >= table_ranges_first_final ? 1 : 0}:#{p}:#{trace}"
 end
end
