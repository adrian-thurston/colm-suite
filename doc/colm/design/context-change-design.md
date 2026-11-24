# Changing a context object from outside its parse: design

How the compiler meets the [requirements](context-change-requirements.md).
The checks run in `src/colm/synthesis.cc` as code is generated. The reverse
code doesn't change.

The setup windows and the hold were replaced by [#771]'s ownership check,
which the [ownership design](context-ownership-design.md) describes. What is
left here, references out and trees into a parse, goes with [#773].

## Contexts and context references

- `StructDef::isContext` is set where `Namespace::declare()` gives a def or
  a token its `contextIn`.
- A *context reference* is a step of a qualification that goes through `->`
  on a value whose type is a context, other than `this`.
  `LangVarRef::lookupQualification()` records the first two in
  `VarRefLookup::firstCtxPart` and `secondCtxPart`, and the variable the
  qualification starts at in `rootField`. `lookupField()` sets `rootField`
  for a name alone.
- The parse's own access, the context's variables by name and `this`, is
  no context reference and isn't checked. At run time a name works only in
  a parse's actions: `parse_context()` in `bytecode.c` stops it elsewhere
  ([#550]).

## References out

`LangVarRef::checkContextValue()` refuses a value reached through a context
reference whose type is a list, a map, a parser, a stream, an input or a
struct that isn't a context (`ctxHeldRef()`), or a tree that shares one
(`sharesRef()`), whether it is a name's value (`evaluate()`) or a builtin
call's result (`evaluateCall()`):
`cannot take L out of C, a context object`, or
`cannot take what find gives out of C, a context object`. `heldRef()` counts
both kinds. A tree shares one if `Compiler::treeHoldsRef()` finds its type:
one with an attribute, or a tree below it with one, that can hold a list,
map, parser, stream or struct. It finds them once, adding each def or token
whose attributes or production children reach one until no more do, and
`any` counts once any type does ([#714]'s check uses it too).

`Compiler::ctxValueInPlace`, which `evaluate()` takes and clears as it
starts, lets three through:

- what a `for` loops over, named, when the loop variable's type isn't one of
  those, a tree that shares one included (`forInPlace()`, from
  `chooseTriterCall()` and `compileForIter()`);
- the parser or stream a send goes to;
- a value read through the same root that a store through a context
  reference puts into the object (`evaluateSetupValue()`).

Field reads (`C->L->length`, `C->T.L->length`, `C->P->tree` of a tree that
shares nothing) and calls that give a value (`find` on a map of `str`) are as
before. Any other use of a tree that shares one, printing it or matching it
included, takes it out. A context object itself is lent and moved as the
[ownership design](context-ownership-design.md) says.

## Trees into a parse

A tree that is sent with `send_tree`, is an item of a `parse_tree`, or is
pushed onto a parser's input goes into the parse as itself. A tree that
shares a list, map, parser, stream or struct is refused there
(`checkTreeIntoParse()`), wherever the code is, and whether the parser can
back up or not:

- a `send_tree` item: `LangTerm::evaluateSendParser()`, when it isn't
  sending strings;
- a `parse_tree` item: `LangTerm::evaluateParse()`, when `tree` is set;
- a `push` or a `push_ignore` onto an input (`IN_INPUT_PUSH_WV`,
  `IN_INPUT_PUSH_IGNORE_WV`), in `evaluateArgs()`, unless it is made
  through `input`, the inbuilt object a token action pushes onto its own
  parser's input with (`isInbuiltObject()`).

The errors:
`cannot send_tree a T, which can hold a list, map, parser, stream or struct: send its text`,
`cannot parse_tree a T, which can hold a list, map, parser, stream or struct: parse its text`
and
`cannot push a T, which can hold a list, map, parser, stream or struct, outside a token action: push its text`.

## Choices

- [#706]'s checks are at compile time, not a stop at run time: the
  maintainer's brief.
- References out are refused, not left to the program: one assignment would
  get past the rest.
- Not taken: a read-only type qualifier, exact about aliases and far larger.
- For trees, only the types that can hold a list, map, parser, stream or
  struct, not every tree: the maintainer's answer. Every tree but `str`
  would have refused twenty programs in `test/colm.d` that set a context's
  tree up or print one after the parse.
- Such a tree goes into a context object only in its parse's actions: the
  maintainer's note on #717, "we don't need to allow trees in and out of a
  context". Printing and matching it take it out, as they do a global's
  ([#714]).
- A tree into a parse is refused wherever it is, not just where the parser
  is restartable: a `send_tree` always goes to a `new parser`, and nothing in
  the tree sends or parses such a tree. A token action's pushes onto `input`
  stay: the C++ grammar's lookup pushes what `make_tree` makes, an `any`,
  which counts once the grammar's `lang_object` attributes do.
- A tree whose attributes can hold a parser, and nothing else, counts too:
  the maintainer's answer when this rule met [#718]'s `sendown1`–`sendown4`.
  Exempting it would let the program and a parser's action both send to the
  parser such a tree carries, and backing up over the action's send would
  take out the wrong text, as [#368] did through a global's parser. With
  [#714]'s rule and the ownership check, no program that compiles reaches [#619]'s
  two stops, a send to a parser while one of its actions runs and backing up
  over one (`parser_check()` and `IN_PARSE_INIT_BKT` in `bytecode.c`). They
  stay as safeguards, and `sendown1`–`sendown4`, which reached them through
  such trees, are removed.

## Limits

- A list, map or struct that a parse's actions take out of their context
  into a global, or put into it from one, can be changed through the global.
  In what a restartable parser runs, the companion rule, [#714], refuses
  both: putting it into a global changes the global, and taking it out of
  one is a take ([design](global-change-design.md)).
- A tree that the program reads with `P->tree`, or gets from `finish()`,
  once the parser's input has ended, shares what the parse's actions put
  into its attributes. A send after the end makes the parser back up over
  them ([#439]).
- The checks go by type: a cast to a tree type that can't hold a list, map,
  parser, stream or struct hides one ([#755]).
- A function that a token action gives `input` to pushes onto an input that
  isn't `input`, and is refused a tree that shares one.

## Tests

In `test/colm.d`. The cases for changes, setup and the hold are listed in
the [ownership design's Tests](context-ownership-design.md#tests).

For trees that share a list: `ctxtake9` is [#720]'s program, and
`ctxtake10`–`ctxtake15` take one out after the parse's action made it, loop
over a list of them, take what `find` gives, put in one whose child holds
the list, put in one a `parse` made, and print one. `treesend1`–`treesend4`
are [#729]'s program, through a global, as a `parse_tree` item, and pushed
onto the input `P->gets()` gives. `ctxset2` runs what stays: reads of such a
tree's attributes in place, after the parser is finished, as the parser keeps
or backs up over its actions. `treesend5` runs a `send_tree`, a
`parse_tree` and a push of a tree that shares nothing, a holder sent as its
text, and a token action's push onto `input`, in a program that has such a
tree type. `attrval1`'s context holds a tree of a type with no list or
struct, which it can set up with a `cons`.
The review of #749 added `ctxtake16`–`ctxtake20`, on `ctxtake10`'s program:
matching such a tree, taking its text with `$`, a tree loop over it, a held
parser's tree of such a type, and a `cons` at setup. It also added
`treesend6`, a `push_ignore`. In `treesend7`, which sends an `any`, and
`ctxtake21`, which sets up a type named `item`, the message puts `an`
before the type's name (`article()`). `treesend8` is the program of the
removed `sendown2`: two parsers that reach each other through parser
attributes of trees sent with `send_tree`.

`ctxtake10`, `ctxtake11`, `ctxtake12` and `ctxtake16`–`ctxtake19` make their
take between two sends, while the object is lent to the parser. Since
[#771], each stops at the use of the lent `C`, `cannot use C: it is lent to
P (...) until P->finish()`, before the take is checked. [#773] sorts them
again.

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#439]: https://github.com/adrian-thurston/colm-suite/issues/439
[#550]: https://github.com/adrian-thurston/colm-suite/issues/550
[#619]: https://github.com/adrian-thurston/colm-suite/issues/619
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#720]: https://github.com/adrian-thurston/colm-suite/issues/720
[#729]: https://github.com/adrian-thurston/colm-suite/issues/729
[#771]: https://github.com/adrian-thurston/colm-suite/issues/771
[#773]: https://github.com/adrian-thurston/colm-suite/issues/773
[#755]: https://github.com/adrian-thurston/colm-suite/issues/755
