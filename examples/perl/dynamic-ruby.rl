%%{
 machine dynamic_jump;
 action jump { fexec 0; fnext *fentry(other); }
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
puts "position=#{p}:#{cs >= dynamic_jump_first_final ? 'ACCEPT' : 'FAIL'}"
