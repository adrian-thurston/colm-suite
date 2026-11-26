# The context the ctxloan cases share. holder's action gives its attribute
# the struct the object holds, so a parse's holder carries it out. The
# parser tries A1 first: the item's action pushes onto R->L, and a newline
# makes the parser back up to A2, over the push.
struct rec
	L: list<int>
end

context ctx
	R: rec
	T: holder

	lex
		ignore /[ \t]+/
		literal `h `w `* `^ `;
		token NL /'\n'/
	end

	def holder
		HR: rec
		[`h]
		{
			lhs.HR = R
		}

	def wrap
		[`w holder]

	def A1 []
	def A2 []

	def item
		[`*]
		{
			R->L->push_tail( 1 )
		}

	def start
		[A1 item* `; `^ NL]
	|	[A2 `* `; NL]
end
