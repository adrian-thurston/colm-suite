%%{
 machine next_hold;
 action jump { fhold; fnext other; }
 main := 'a' @jump;
 other := 'a';
}%%
%% write data;
data = 'a'
p = 0
pe = data.length
cs = 0
%% write init;
%% write exec;
puts "position=#{p}:#{cs >= next_hold_first_final ? 'ACCEPT' : 'FAIL'}"
