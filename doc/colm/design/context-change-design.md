# Changing a context object from outside its parse: design

How the compiler meets the [requirements](context-change-requirements.md).
The checks run in `src/colm/synthesis.cc` as code is generated. Nothing in
the bytecode, the reverse code or the runtime changes.

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

## Setup windows

A `Compiler::SetupWindow` is open while a variable's object is being set up.

- **It opens** after an assignment or a definition that puts a `new` object
  of a context into a variable named alone, local or global
  (`LangStmt::compile()`, `AssignType`).
- **It covers** the rest of its statement list, if and else branches
  included (`compileStmtList()`), but no loop body it is outside of
  (`compileLoopBody()` counts `loopDepth`), and no `while` loop's test
  (`compileWhile()` evaluates it a loop deeper): the body runs again after
  what follows it, and the test after the body. Each code block starts with
  none (`CodeBlock::compile()`).
- **It ends at once** where the variable is used as a value, in
  `LangVarRef::evaluate()` (`closeSetupWindow()`): given to a parse, a
  function or another variable, returned or tested.
- **It ends with its statement** where the variable is a `new parser`'s
  context (`newParserContext`): a new parser runs nothing yet. Other code
  that runs later in the statement could send to it, and ends the setup
  there (`endReachableSetups()`).
- **It ends** where the variable is assigned again, opening a new one if the
  value is `new`, and where its statement list ends.
- **For a variable that isn't a local** (a global, or a context's variable
  by name), it also ends where other code runs, which could reach the
  variable, give the object to a parser and send to it
  (`endReachableSetups()`): a call of a user or host function, a parse, a send
  to a parser, a parser's `finish`, a user iterator's loop, and a `yield`,
  which runs the loop's body.

## Changes

`LangVarRef::checkContextChange()` checks a change through a context
reference. It compiles only if its first context step is the variable at the
root, that variable's window is open, and no second context step follows: a
context object that another holds isn't set up through it. The change is
checked where it runs, after its arguments, its right-hand side or its send
items, any of which can end the window:

- an assignment: `LangVarRef::assignValue()`;
- a builtin call that isn't const, or a parser's `finish`:
  `LangVarRef::evaluateCall()`;
- a send or a `send_tree` to a parser or a stream:
  `LangTerm::checkSendChange()`, from `evaluateSend()` and
  `evaluateSendTree()`.

The error is at the context step:
`cannot change C, a context object, outside its setup`.

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
- the parser or stream a send goes to, which is checked as a change;
- in a window, a value read through its variable that a change through the
  same variable puts into the object.

Field reads (`C->L->length`, `C->T.L->length`, `C->P->tree` of a tree that
shares nothing) and calls that give a value (`find` on a map of `str`) are as
before. Any other use of a tree that shares one, printing it or matching it
included, takes it out. A context object itself is free to pass: every use of
it is checked in turn.

## What goes in at setup

In a window, `evaluateSetupValue()` checks what a change through the
variable puts into the object: an assignment's right-hand side
(`setupStoreVar`, set in `AssignType`), or a changing call's arguments (set
in `evaluateCall()`, checked in `evaluateArgs()`). A list, map, parser,
stream, input or non-context struct must be a `new` term, `nil`, or a name
read through the same variable:
`only a new object, nil or what C holds can go into C as it is set up`.
A tree that shares one must be `nil` or a name read through the variable: a
`cons`, a `parse`, `make_token` or `make_tree` can put into it what is held
elsewhere, as a reduction action's `lhs.L = GL` does. The error names the
tree's type:
`only nil or what C holds can go into C as it is set up: a T can hold a list, map, parser, stream or struct`.

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

- A compile-time check, not a stop at run time: the maintainer's brief.
- Setup windows, not first values or an init block in the context: existing
  setup compiles unchanged, and no syntax is added.
- References out are refused, not left to the program: one assignment would
  get past the rest.
- A global gets a window too, ended by other code, rather than windows for
  locals alone: a global set up after `new` is natural code.
- Kept simple at a small cost: `if ( C->R )` is refused (`C->R == nil`
  doesn't compile anyway), and a send to a held stream is a change, though
  streams aren't undone.
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

## Limits

- Two parsers that the program drives, given one object, change it under
  each other ([#718]). Only a parser that owns its context could rule that
  out.
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
  parser, stream or struct hides one.
- A function that a token action gives `input` to pushes onto an input that
  isn't `input`, and is refused a tree that shares one.

## Tests

In `test/colm.d`. `ctxset1` runs what a setup may do, and reads the object
in place after its parse. `ctxchg1`–`ctxchg18` (changes),
`ctxtake1`–`ctxtake8` (references out, and what goes in) and `undoctx4` are
compile errors, one each, and say in their first lines what they show.
`ctxchg1` and `ctxchg2` are [#706]'s programs, and `ctxchg4` is [#368]'s
form. The review of #717 added `ctxchg15`–`ctxchg18`: a call later in a new
parser's statement, a function the object goes through into one, a while
loop's condition, and a yield. Most include `ctxrule.lmi`, a context to
change.

For trees that share a list: `ctxtake9` is [#720]'s program, and
`ctxtake10`–`ctxtake15` take one out after the parse's action made it, loop
over a list of them, take what `find` gives, put in one whose child holds
the list, put in one a `parse` made, and print one. `treesend1`–`treesend4`
are [#729]'s program, through a global, as a `parse_tree` item, and pushed
onto the input `P->gets()` gives. `ctxset2` runs what stays: reads of such a
tree's attributes in place, between two sends and after the parse, as the
parser keeps or backs up over its actions. `treesend5` runs a `send_tree`, a
`parse_tree` and a push of a tree that shares nothing, a holder sent as its
text, and a token action's push onto `input`, in a program that has such a
tree type. `attrval1`'s context holds a tree of a type with no list or
struct, which it can set up with a `cons`.
The review of #749 added `ctxtake16`–`ctxtake20`, on `ctxtake10`'s program:
matching such a tree, taking its text with `$`, a tree loop over it, a held
parser's tree of such a type, and a `cons` at setup. It also added
`treesend6`, a `push_ignore`. In `treesend7`, which sends an `any`, and
`ctxtake21`, which sets up a type named `item`, the message puts `an`
before the type's name (`article()`).

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#439]: https://github.com/adrian-thurston/colm-suite/issues/439
[#550]: https://github.com/adrian-thurston/colm-suite/issues/550
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#720]: https://github.com/adrian-thurston/colm-suite/issues/720
[#729]: https://github.com/adrian-thurston/colm-suite/issues/729
