# The context the ctxchg and ctxtake cases share.

struct rec
	N: int
end

context ctx
	N: int
	L: list<str>
	R: rec
	T: w
	M: map<str, rec>
	Recs: list<rec>
	Inner: accum<inner>

	lex
		token w /[a-z]+/
		ignore /[ \n]+/
	end

	def inner
		[w*]

	def start
		[w*]
end
