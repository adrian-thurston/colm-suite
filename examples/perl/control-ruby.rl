%%{
 machine control;
 action stop { fbreak; }
 main := 'abc' @stop 'd';
}%%
%% write data;
data = 'abcd'
p = 0
pe = data.length
cs = 0
%% write init;
%% write exec;
puts "position=#{p}"
