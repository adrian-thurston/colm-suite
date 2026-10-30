%%{
    machine greeting;
    action begin_name { name << '[' }
    action remember { name << fc.chr }
    main := 'HELLO:' ([A-Z]+ >begin_name $remember) '\n';
}%%

%% write data;
["HELLO:ALICE\n", "HELLO:\n", "HELLO:alice\n"].each do |data|
  p = 0
  pe = data.length
  cs = 0
  name = ''
  %% write init;
  %% write exec;
  puts "#{cs >= greeting_first_final ? 'ACCEPT' : 'FAIL'}:#{name}"
end
