%%{
 machine jump;
 action jump_now { trace << 'J'; fgoto *fentry(other); trace << 'X'; }
 main := 'a' @jump_now;
 other := 'b';
}%%
%% write data;
data = 'ab'
p = 0
pe = data.length
cs = 0
trace = ''
%% write init;
%% write exec;
puts "#{trace}:#{p}:#{cs >= jump_first_final ? 'ACCEPT' : 'FAIL'}"
