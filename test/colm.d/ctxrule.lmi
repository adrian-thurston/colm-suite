# The context the ctxchg and ctxtake cases share.

struct rec
	N: int
end

# A tree whose attribute holds a list: a copy of it shares the list.
lex
	literal `h
end

def holder
	L: list<str>
	[`h]

def box
	[holder]

context ctx
	N: int
	L: list<str>
	R: rec
	T: w
	M: map<str, rec>
	Recs: list<rec>
	Inner: accum<inner>
	H: holder
	B: box

	lex
		token w /[a-z]+/
		ignore /[ \n]+/
	end

	def inner
		[w*]

	def start
		[w*]
end
