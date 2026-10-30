%%{
 machine conditions;
 action flag_one { one }
 action flag_two { two }
 action seen_one { trace << '1' }
 action seen_two { trace << '2' }
 main := ('a' 'b' when flag_one @seen_one | 'a' 'b' when flag_two @seen_two);
}%%
%% write data;
[[false,false], [true,false], [false,true], [true,true]].each do |one, two|
 data = 'ab'
 p = 0
 pe = data.length
 cs = 0
 trace = ''
 %% write init;
 %% write exec;
 puts "#{trace}:#{cs >= conditions_first_final ? 'ACCEPT' : 'FAIL'}"
end
