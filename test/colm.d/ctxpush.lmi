# The context ctxown28-42 share. The parser tries A1 first: the item's action
# pushes onto L, and a newline makes the parser back up to A2, over the push.
context ctx
	L: list<str>

	lex
		ignore /[ \t]+/
		literal `* `^ `; `-
		token NL /'\n'/
	end

	def A1 []
	def A2 []

	def item
		[`*]
		{
			L->push_tail( "x" )
		}

	def start
		[A1 item* `; `^ NL]
	|	[A2 `* `; NL]
end
