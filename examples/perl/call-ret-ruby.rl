%%{
 machine call_ret;
 action call_sub { trace << 'C'; fcall submachine; trace << 'X'; }
 action return_sub { trace << 'R'; fret; trace << 'X'; }
 main := 'a' @call_sub 'c';
 submachine := 'b' @return_sub;
}%%
%% write data;
data = 'abc'
p = 0
pe = data.length
cs = 0
top = 0
stack = []
trace = ''
%% write init;
%% write exec;
puts "#{trace}:#{p}:#{cs >= call_ret_first_final ? 'ACCEPT' : 'FAIL'}"
