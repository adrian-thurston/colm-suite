%%{
 machine state_actions;
 action enter { trace << 'T' }
 action leave { trace << 'F' }
 action transition { trace << 'X' }
 action ended { trace << 'E' }
 main := ('ab' $to enter $from leave $transition) %eof ended;
}%%
%% write data;
['ab', 'a', 'x', ''].each do |data|
 p = 0
 pe = data.length
 eof = pe
 cs = 0
 trace = ''
 %% write init;
 %% write exec;
 puts "#{trace}:#{cs >= state_actions_first_final ? 'ACCEPT' : 'FAIL'}"
end
