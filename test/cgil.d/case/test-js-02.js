'use strict';


var _test_actions = new Int8Array([ 0, 1, 0, 0 ]);
var _test_trans_keys = new Uint8Array([ 1, 0, 0, 0, 0, 1, 1, 0, 0]);
var _test_char_class = new Int8Array([ 0, 1, 0 ]);
var _test_index_offsets = new Int8Array([ 0, 0, 1, 3, 0 ]);
var _test_indices = new Int8Array([ 2, 2, 3, 0 ]);
var _test_index_defaults = new Int8Array([ 0, 0, 0, 0, 0 ]);
var _test_cond_targs = new Int8Array([ 0, 1, 2, 3, 3, 0 ]);
var _test_cond_actions = new Int8Array([ 0, 0, 0, 1, 0, 0 ]);
var test_start  = 1;
var test_first_final  = 3;
var test_error  = 0;
var test_en_main  = 1;

{
	cs = test_start;
	
}

{
	var _trans 
	= 0;
	var _keys;
	var _acts;
	var _inds;
	var _nacts
	;
	var _ic
	;
	_resume: while ( p !==pe  ) {
		_again: while ( true  ) {
			_keys = (cs<<1) ;
			_inds = _test_index_offsets[cs] ;
			if ( ( data.charCodeAt(p )) <= 98 && ( data.charCodeAt(p )) >= 97  )
			{
				_ic = _test_char_class[( data.charCodeAt(p )) - 97];
				if ( _ic <= _test_trans_keys[_keys+1 ]&& _ic >= _test_trans_keys[_keys ] )
				_trans = _test_indices[_inds + ( _ic - _test_trans_keys[_keys ])  ];
				
				else
				_trans = _test_index_defaults[cs];
				
				
			}
			
			else
			{
				_trans = _test_index_defaults[cs];
				
			}
			
			cs = _test_cond_targs[_trans];
			if ( _test_cond_actions[_trans] !==0  )
			{
				_acts = _test_cond_actions[_trans] ;
				_nacts = _test_actions[_acts ];
				_acts += 1;
				while ( _nacts > 0  )
				{
					switch ( _test_actions[_acts ] ) {
						case 0 :
						{
							n += 1; 
						}
						
						break;
						
					}
					_nacts -= 1;
					_acts += 1;
					
				}
				
				
			}
			
			break _again;
			
		}
		if ( cs !==0  )
		{
			p += 1;
			continue _resume;
			
		}
		
		break _resume;
		
	}
	
}

