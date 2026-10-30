%%{
 machine overrides;
 variable p pos;
 variable pe end_pos;
 variable eof end_of_input;
 variable cs state;
 variable data input;
 action seen { trace << fc.chr }
 action ended { trace << '!' }
 main := ('ab' $seen) %eof ended;
}%%
%% write data;
input = 'ab'
pos = 0
end_pos = input.length
end_of_input = end_pos
state = 0
trace = ''
%% write init;
%% write exec;
puts "#{trace}:#{pos}:#{state >= overrides_first_final ? 'ACCEPT' : 'FAIL'}"
