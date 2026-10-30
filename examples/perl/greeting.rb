




class << self
	attr_accessor :_greeting_actions
	private :_greeting_actions, :_greeting_actions=
end
self._greeting_actions = [
	0, 1, 1, 2, 0, 1
]

class << self
	attr_accessor :_greeting_key_offsets
	private :_greeting_key_offsets, :_greeting_key_offsets=
end
self._greeting_key_offsets = [
	0, 0, 1, 2, 3, 4, 5, 6,
	8, 11
]

class << self
	attr_accessor :_greeting_trans_keys
	private :_greeting_trans_keys, :_greeting_trans_keys=
end
self._greeting_trans_keys = [
	72, 69, 76, 76, 79, 58, 65, 90,
	10, 65, 90, 0
]

class << self
	attr_accessor :_greeting_single_lengths
	private :_greeting_single_lengths, :_greeting_single_lengths=
end
self._greeting_single_lengths = [
	0, 1, 1, 1, 1, 1, 1, 0,
	1, 0
]

class << self
	attr_accessor :_greeting_range_lengths
	private :_greeting_range_lengths, :_greeting_range_lengths=
end
self._greeting_range_lengths = [
	0, 0, 0, 0, 0, 0, 0, 1,
	1, 0
]

class << self
	attr_accessor :_greeting_index_offsets
	private :_greeting_index_offsets, :_greeting_index_offsets=
end
self._greeting_index_offsets = [
	0, 0, 2, 4, 6, 8, 10, 12,
	14, 17
]

class << self
	attr_accessor :_greeting_trans_targs
	private :_greeting_trans_targs, :_greeting_trans_targs=
end
self._greeting_trans_targs = [
	2, 0, 3, 0, 4, 0, 5, 0,
	6, 0, 7, 0, 8, 0, 9, 8,
	0, 0, 0
]

class << self
	attr_accessor :_greeting_trans_actions
	private :_greeting_trans_actions, :_greeting_trans_actions=
end
self._greeting_trans_actions = [
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 3, 0, 0, 1,
	0, 0, 0
]

class << self
	attr_accessor :greeting_start
end
self.greeting_start = 1;
class << self
	attr_accessor :greeting_first_final
end
self.greeting_first_final = 9;
class << self
	attr_accessor :greeting_error
end
self.greeting_error = 0;

class << self
	attr_accessor :greeting_en_main
end
self.greeting_en_main = 1;


["HELLO:ALICE\n", "HELLO:\n", "HELLO:alice\n"].each do |data|
  p = 0
  pe = data.length
  cs = 0
  name = ''

begin
	p ||= 0
	pe ||= data.length
	cs = greeting_start
end


begin
	_klen, _trans, _keys, _acts, _nacts = nil
	_goto_level = 0
	_resume = 10
	_eof_trans = 15
	_again = 20
	_test_eof = 30
	_out = 40
	while true
	_trigger_goto = false
	if _goto_level <= 0
	if p == pe
		_goto_level = _test_eof
		next
	end
	if cs == 0
		_goto_level = _out
		next
	end
	end
	if _goto_level <= _resume
	_keys = _greeting_key_offsets[cs]
	_trans = _greeting_index_offsets[cs]
	_klen = _greeting_single_lengths[cs]
	_break_match = false

	begin
	  if _klen > 0
	     _lower = _keys
	     _upper = _keys + _klen - 1

	     loop do
	        break if _upper < _lower
	        _mid = _lower + ( (_upper - _lower) >> 1 )

	        if data[p].ord < _greeting_trans_keys[_mid]
	           _upper = _mid - 1
	        elsif data[p].ord > _greeting_trans_keys[_mid]
	           _lower = _mid + 1
	        else
	           _trans += (_mid - _keys)
	           _break_match = true
	           break
	        end
	     end # loop
	     break if _break_match
	     _keys += _klen
	     _trans += _klen
	  end
	  _klen = _greeting_range_lengths[cs]
	  if _klen > 0
	     _lower = _keys
	     _upper = _keys + (_klen << 1) - 2
	     loop do
	        break if _upper < _lower
	        _mid = _lower + (((_upper-_lower) >> 1) & ~1)
	        if data[p].ord < _greeting_trans_keys[_mid]
	          _upper = _mid - 2
	        elsif data[p].ord > _greeting_trans_keys[_mid+1]
	          _lower = _mid + 2
	        else
	          _trans += ((_mid - _keys) >> 1)
	          _break_match = true
	          break
	        end
	     end # loop
	     break if _break_match
	     _trans += _klen
	  end
	end while false
	cs = _greeting_trans_targs[_trans]
	if _greeting_trans_actions[_trans] != 0
		_acts = _greeting_trans_actions[_trans]
		_nacts = _greeting_actions[_acts]
		_acts += 1
		while _nacts > 0
			_nacts -= 1
			_acts += 1
			case _greeting_actions[_acts - 1]
when 0 then
		begin
 name << '[' 		end
when 1 then
		begin
 name << data[p].ord.chr 		end
			end # action switch
		end
	end
	if _trigger_goto
		next
	end
	end
	if _goto_level <= _again
	if cs == 0
		_goto_level = _out
		next
	end
	p += 1
	if p != pe
		_goto_level = _resume
		next
	end
	end
	if _goto_level <= _test_eof
	end
	if _goto_level <= _out
		break
	end
	end
	end

  puts "#{cs >= greeting_first_final ? 'ACCEPT' : 'FAIL'}:#{name}"
end
