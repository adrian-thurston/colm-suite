# Changing a context object from outside its parse: design

How the compiler meets the [requirements](context-change-requirements.md).
The checks run in `src/colm/synthesis.cc` as code is generated, but for one,
that a parser holds its context object, which runs with the program
([One parser per object](#one-parser-per-object)). The reverse code doesn't
change.

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
struct that isn't a context (`ctxHeldRef()`), whether it is a name's value
(`evaluate()`) or a builtin call's result (`evaluateCall()`):
`cannot take L out of C, a context object`, or
`cannot take what find gives out of C, a context object`.

`Compiler::ctxValueInPlace`, which `evaluate()` takes and clears as it
starts, lets three through:

- what a `for` loops over, named, when the loop variable's type isn't one of
  those (`forInPlace()`, from `chooseTriterCall()` and `compileForIter()`);
- the parser or stream a send goes to, which is checked as a change;
- in a window, a value read through its variable that a change through the
  same variable puts into the object.

Field reads (`C->L->length`, `C->P->tree`) and calls that give a value
(`find` on a map of `str`) are as before. A context object itself is free to
pass: every use of it is checked in turn.

## What goes in at setup

In a window, `evaluateSetupValue()` checks what a change through the
variable puts into the object: an assignment's right-hand side
(`setupStoreVar`, set in `AssignType`), or a changing call's arguments (set
in `evaluateCall()`, checked in `evaluateArgs()`). A list, map, parser,
stream, input or non-context struct must be a `new` term, `nil`, or a name
read through the same variable:
`only a new object, nil or what C holds can go into C as it is set up`.

## One parser per object

Two parsers given one object change it under each other ([#718]), and no
check of the code can see which object a reference holds. So requirement 8
is checked when the program runs:

- A context object has one slot more than its fields, which names the
  parser that holds it, or is nil. `makeRuntimeData()` (`pdabuild.cc`) adds
  the slot to the size of a context in `sel_info`, and `context_hold()`
  (`bytecode.c`) finds it, last.
- A parser holds what it is given for its owner, the parser whose backing up
  backs it up. `IN_SET_PARSER_OWNER`, right after a parser is made, takes a
  byte saying which parser it is: `PARSER_NEW` from `evaluateNew()`,
  `PARSER_PARSE` from `evaluateParse()`, or `PARSER_RESTART_PARSE`, which
  `evaluateParse()` writes over it where an item after the first runs other
  code, as it puts the type in `restartTypes`.
  - A `PARSER_PARSE` in an action (`exec->parser`) runs inside the action,
    and is backed up with it. It names the owner of the parser the action
    belongs to, in `pda_run->hold_owner`.
  - Every other parser is its own owner, and holds while
    `pda_run->holding` is set. A parse with an item after the first that
    runs code can back up between its items, while the action runs, so it
    holds for itself.
  - `parser_owner()` follows `hold_owner` to the parser that names itself.
    `colm_parser_new()` makes a parser its own owner, holding, until
    `IN_SET_PARSER_OWNER` runs, so a parser made another way holds to the
    end of the program.
- `IN_SET_PARSER_CONTEXT` stops the program, with
  `giving a parser a context that another parser holds`, when the slot names
  a parser whose owner isn't the given parser's and is holding. Otherwise
  it names the given parser's owner in the slot. Parsers with one owner
  share the object: the owner backs their actions up last first.
- `IN_RELEASE_PARSER_CONTEXT`, after every parse's last fragment, ends the
  hold of a parse that held for itself. Outside any action (`exec->parser`
  is nil), it clears `holding`, and what the parse and its actions' parses
  were given is free. In an action, backing up over the action can still
  back the parse up, so it names the action's owner instead, which holds
  what it held. A parser made with `new` holds to the end of the program:
  text sent to it after its end parses again, and backs up ([#439]).
- A parse can be given one context. A second one, which would replace the
  first and leave it held, is a compile error in `evaluateParse()`:
  `parse command requires just context and input`, as `new parser` already
  refuses one.

Not taken:

- A compile-time rule that a parser is given only an object being set up,
  through the variable it was made in. It refuses more than sharing: an
  object set up in a function and returned, two parses one after the other,
  a parameter, `this` through a function, and a nil variable.
- A parser that owns its context, made with it: a language change.
- Sharing with a parser whose actions never change their context: it needs
  a pass over what each parser runs, for a reader of another parse's data.
- The manual's old exceptions, a parser that a parse's actions send to, and
  any parse they start: the first crashes the same way (`ctxhold3`), and so
  does the second when an item of it after the first runs code (`ctxhold4`).
- A parse in an action holding its object for good, and sharing only
  `this`, as #750 first had it: it stopped programs that run correctly, an
  action that parses an object of another context each time it runs, and a
  parse of that object after the outer parse has ended (`ctxhold8`).

This is the one check that runs with the program. The maintainer chose it
for [#718] over the compile-time rule above, which refuses too much.

## Choices

- [#706]'s checks are at compile time, not a stop at run time: the
  maintainer's brief. [#718]'s is the one exception
  ([One parser per object](#one-parser-per-object)).
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

## Limits

- A parser given the object it is held in, as in
  `C->Inner = new parser<ctx::inner>( C )`, can't be run: the program can't
  send to it after the setup, and no other parser can be given the object.
- A list, map or struct that a parse's actions take out of their context
  into a global, or put into it from one, can be changed through the global.
  In what a restartable parser runs, the companion rule, [#714], refuses
  both: putting it into a global changes the global, and taking it out of
  one is a take ([design](global-change-design.md)).
- Trees go into and out of a context object freely, as values, but a copy
  of a tree shares the list, map, parser, stream or struct that an attribute
  of it, or of a tree below it, holds. So the program can change the
  context's data through a tree it took out or put in and kept, and no
  check sees it ([#720]).

## Tests

In `test/colm.d`. `ctxset1` runs what a setup may do, and reads the object
in place after its parse. `ctxchg1`–`ctxchg18` (changes),
`ctxtake1`–`ctxtake8` (references out, and what goes in) and `undoctx4` are
compile errors, one each, and say in their first lines what they show.
`ctxchg1` and `ctxchg2` are [#706]'s programs, and `ctxchg4` is [#368]'s
form. The review of #717 added `ctxchg15`–`ctxchg18`: a call later in a new
parser's statement, a function the object goes through into one, a while
loop's condition, and a yield. Most include `ctxrule.lmi`, a context to
change. `ctxhold1`–`ctxhold5` give one object to a second parser, which stops
the program: [#718]'s program and its map form, a parser the context holds,
a parse an action starts with an item that pops the list, and in `ctxhold5`
a parse between two sends, a parse in a function, a new parser given `this`,
and a parse after a finished parser. `ctxholdmsg1` shows the message.
`ctxhold6` runs what still shares: two parses one after the other, and a
parse a token action starts, given `this`. `ctxhold7` is a parse in a new
parser's action, given a global, which holds it while the parser can back
up. `ctxhold8` runs parses an outer parse's actions start, given one object
of another context: for each item, after the outer parse has ended, and
after backing up. `ctxhold9` stops a parse given an object that a new
parser without a context holds for its action's parse, and one in an item
of a parse that holds for itself, and runs the parses after that one has
ended. `noctx6` is a parse given two contexts.

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#439]: https://github.com/adrian-thurston/colm-suite/issues/439
[#550]: https://github.com/adrian-thurston/colm-suite/issues/550
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#720]: https://github.com/adrian-thurston/colm-suite/issues/720
