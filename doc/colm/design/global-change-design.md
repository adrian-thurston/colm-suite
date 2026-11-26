# Changing a global in a restartable parser: design

How the compiler meets the [requirements](global-change-requirements.md).
The check runs in `src/colm/synthesis.cc`, in two parts. As each block is
compiled, what its revert code does to globals is noted with it. Once all the
code is compiled, `Compiler::checkRestartable()` finds what restartable
parsers run and refuses what is noted there, and finds what parses given a
context run and refuses there what [#770] does: a take, or a change that puts
a reference value in. Nothing in the bytecode, the reverse code or the
runtime changes.

## Noting what a block does

- `Compiler::curBlock` is the code block being compiled: a production's
  action (`compileReductionCode()`), a token action
  (`compileTranslateBlock()`), a preeof block (`compilePreEof()`), a function
  (`compileFunction()`) or an iterator (`compileUserIter()`). It is 0 for the
  root block. A function or an iterator is compiled twice. What is noted
  comes from its revert compile (`revertOn`), the code that actions run.
- `LangVarRef::noteGlobalUse()` keeps a `GlobalUse` in the block's
  `globalUses` when the reference starts at a global
  (`LangVarRef::isGlobalRef()`, which makes `loadObj()`'s choice). A child
  named on its own in a switch case starts where the switch's variable
  does, and the use names the global that variable starts at. These are:
  - a change: in `assignValue()`; in `evaluateCall()`, for a call on what a
    global holds that changes it (its `change`, which counts a parser's
    `finish`); and in `LangTerm::checkSendChange()`, for a send to a
    parser, not to a stream;
  - a take: in `LangVarRef::evaluate()`, of a value that `heldRef()` counts,
    unless it is read in place (`ctxValueInPlace`), and in `evaluateCall()`,
    of such a value that a call on what a global holds gives, and in
    `sendValueKept()`, of a send's target, a parser or a stream, when the
    send's value is kept: the target is read in place, but the value is what
    was sent to. `heldRef()` counts what `checkContextValue()` counts: what
    `ctxHeldRef()` does, and a tree type that `Compiler::treeHoldsRef()`
    finds (`sharesRef()`): one with an attribute, or a tree below it with
    one, that can hold a list, map, parser, stream or struct. It finds them
    once, adding each def or token whose attributes or production children
    reach one until no more do, and `any` counts once any type does. A `for`
    loop over a list or a map of such trees doesn't read it in place
    (`forInPlace()`): each value is a copy that shares what its attributes
    hold, so the list or the map is taken.
- A change notes whether it puts a reference value in
  (`GlobalUse::putsRef`): a value that `heldRef()` counts. In
  `assignValue()` it is the assigned value's type. In `evaluateCall()` it is
  an argument's (`argHeldRef()`), whose type `evaluateArgs()` keeps in
  `CallArg::exprUT`: a built-in's arguments are values. In
  `checkSendChange()` it is a stream sent to the parser, which the parser's
  input then holds, as `evaluateSendParser()` reports. It goes by type, so a
  `new` object counts.
- `evaluateCall()` and `compileForIter()` note the functions and iterators a
  block calls (`CodeBlock::calls`), and `evaluateParse()` the types it parses
  (`CodeBlock::parses`).

## Restartable types

`Compiler::restartTypes` holds the types that restartable parsers parse:

- `LangTerm::evaluateNew()`: the type of every `new parser`.
- `LangTerm::evaluateParse()`: the type of a parse with an item after the
  first that runs other code. `Compiler::otherCodeRuns` counts the calls of
  `countOtherCode()`, made wherever other code runs: a call of a function, a
  parse, a send to a parser, a `finish`, a user iterator's loop and a yield.
  `evaluateCall()` counts a call that changes what it is called on, such as
  `pop_tail`, too. An item whose evaluation changes the count runs code.

## What a restartable parser runs

`Compiler::checkRestartable()`, at the end of `compileByteCode()`, walks out
from `restartTypes`, queueing each type and block once (`RestartReach`):

- From a type: the action of each of its productions, and the type of each
  element of each; a token's action, and its lex block, which is its
  `tokenDef->regionSet`. From a lex block: its preeof block, which is kept on
  its `tokenIgnore` region, and every token and ignore defined in it. The
  scanner runs the action of whichever it matches, whether the grammar uses
  it or not.
- From a block: the blocks of the functions and iterators it calls, and the
  types it parses. Backing up over an action backs up a parse it started
  (`IN_PARSE_FRAG_BKT`).

`RestartReach::walk()` takes these steps until nothing more is queued.

## What a parse given a context runs

A parse given a context is one of a type defined in a context, whose
`LangEl::contextIn` is set: `checkContextGiven()` makes each parse of it, and
each `new parser`, be given an object of the context. Its actions can put a
list a global holds into the object (`L = GL`), or a list the object holds
into a global (`GL = L`). A parse that isn't restartable can still do both,
and once the object is lent to a `new parser` whose actions change the list,
the program can change it through the global between sends.

`checkRestartable()` walks out a second time, from every type defined in a
context, in the same steps (`RestartReach::walk()`). In what that walk
reaches, a take is refused, and a change that puts a reference value in.
Other changes stay, and backing up undoes them.

## Reporting

`checkRestartable()` then goes over the blocks in the order
`compileByteCode()` compiled them, functions and iterators, productions,
tokens, preeof blocks, and reports the first `GlobalUse` that the first
block's rule refuses (`reportGlobalUses()`), so the error is the same one
every time. A block that a restartable parser runs is under its whole rule,
whether a parse given a context runs it too or not. A block that only a
parse given a context runs is under that parse's rule.

## Errors

The error is at the global's name:

- a change: `cannot change G, a global, in an action of a restartable parser: keep the data in a context`;
- a take of the global itself, of a field of it, and of what a call on it
  gives: `cannot take G, a global, out in ...`,
  `cannot take L out of G, a global, in ...` and
  `cannot take what find gives out of G, a global, in ...`;
- in a function or an iterator `f`, the error says
  `in f, which an action of a restartable parser runs` in place of
  `in an action of a restartable parser`;
- in what only a parse given a context runs, the errors say
  `in an action of a parse given a context` and
  `in f, which an action of a parse given a context runs`. A change that
  puts a reference value in is `cannot change G, a global, in ...`, as any
  change is.

## Choices

- Restartable parsers only, not every parser: the maintainer's brief. Ragel's
  parser and the manual's lexing examples change globals in parses that
  aren't restartable.
- At compile time, as the ownership check of a context object is, once all
  the code is compiled: what a parser runs is known only then, as for
  `checkRefCopies()`.
- A parse's first item may run code: nothing has been sent when it runs.
- Sends to and `finish` of a parser that a global holds, and takes, are
  refused too: the maintainer's answer. A send to a stream isn't: what goes
  to a stream isn't undone.
- An action's code is compiled once, so once a restartable parser runs it, it
  can't change a global for any parse.
- The lex block, not the grammar, says which token actions a parser runs.
- Kept simple at a small cost: the walk takes every production of a type it
  reaches, and every token of a lex block, whether the parser's states can
  reach them or not.
- Not taken: a run-time stop in the reverse code's global loads. It would
  need the parser's kind at run time, and a mark on each pointer loaded from
  a global, and would stop only when the action runs.
- A tree that can hold a list, map or struct below it counts as a take, as
  it does for a context object ([#720]). Kept simple at a small cost:
  printing or matching such a tree, which uses it as a value, counts too.
- In a parse given a context, restartable or not, takes and changes that
  put a reference value in are refused ([#770]), as requirement 13 of
  [owning a context object](context-ownership-requirements.md) asks.
- What goes into a global goes by type, and a `new` object counts: the
  maintainer's answer. Nothing else holds it yet, but telling it apart
  helps little until a local holding only a new object can be told apart
  too. `attrval1` sets its global's tree in a parse given no context.
- A stream sent to a parser that a global holds puts it in: the
  maintainer's answer. The parser's input holds it, and the program's sends
  to that parser would read from the context object's stream while a parser
  lent the object can back up over its own reads.
- Every type defined in a context starts the second walk, parsed or not.
  The walk takes the defs and tokens outside the context that its grammar
  uses, and their actions, which then can't take out or put in in a parse
  given no context either: an action's code is compiled once.

## Tests

In `test/colm.d`. `globchg1`–`globchg17` (changes) and
`globtake1`–`globtake6` (takes) are compile errors, one each, and say in
their first lines what they show. `globchg1`, `globchg2` and `globchg9` are
[#687]'s, [#683]'s and [#368]'s programs, through a global. `globchg3` is a
parse whose second item calls a function, and `globchg16` one whose second
item pops the list. `globchg17` changes a global through a child named on
its own in a switch case. `globtake5` and `globtake6` take a tree whose
attribute holds a list out of a global, into a variable and as a loop over
a list of them. `globread1` runs what stays: reads in place and a send to a
stream in a restartable parser, and a change undone in a parse whose only
item that runs code is its first. `globread2` reads such a tree's
attributes in place.
`globctx1`–`globctx6` are [#770]'s compile errors in parses given a context
that aren't restartable. `globctx1` and `globctx2` take a global's list into
the object and put the object's list into a global, and lend the object to
a `new parser` whose push the program pops through the global between sends:
both segfaulted before the check. `globctx3` takes in a function an action
calls, `globctx4` pushes the object's struct onto a global's list,
`globctx5` sends the object's stream to a global's parser, and `globctx6`
takes in the action of a def outside the context that its grammar uses.
`globread3` runs what stays there: a global int changed, a str pushed onto a
global's list, and the list read in place, undone when the parse backs up.
`ctxsend3` and `ctxsend4` ([#772]) keep the value of a send to a global's
parser and stream there, which is a take.
`nilset2` and `undomap10` became parses that aren't restartable, and
`sendown1`–`sendown4` kept their parsers in a context, until [#718]'s hold,
which [#771] replaced, made a parser hold its context object. Then each
parser reached another through an attribute of a tree sent to it, until
[#720]'s rule refused a tree that can hold a parser too, and the four were
removed: no program that compiles reaches the stops they tested.

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#683]: https://github.com/adrian-thurston/colm-suite/issues/683
[#687]: https://github.com/adrian-thurston/colm-suite/issues/687
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#720]: https://github.com/adrian-thurston/colm-suite/issues/720
[#770]: https://github.com/adrian-thurston/colm-suite/issues/770
[#771]: https://github.com/adrian-thurston/colm-suite/issues/771
[#772]: https://github.com/adrian-thurston/colm-suite/issues/772
