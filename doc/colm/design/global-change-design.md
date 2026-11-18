# Changing a global in a restartable parser: design

How the compiler meets the [requirements](global-change-requirements.md).
The check runs in `src/colm/synthesis.cc`, in two parts. As each block is
compiled, what its revert code does to globals is noted with it. Once all the
code is compiled, `Compiler::checkRestartable()` finds what restartable
parsers run, and refuses what is noted there. Nothing in the bytecode, the
reverse code or the runtime changes.

## Noting what a block does

- `Compiler::curBlock` is the code block being compiled: a production's
  action (`compileReductionCode()`), a token action
  (`compileTranslateBlock()`), a preeof block (`compilePreEof()`), a function
  (`compileFunction()`) or an iterator (`compileUserIter()`). It is 0 for the
  root block. A function or an iterator is compiled twice. What is noted
  comes from its revert compile (`revertOn`), the code that actions run.
- `LangVarRef::noteGlobalUse()` keeps a `GlobalUse` in the block's
  `globalUses` when the reference starts at a global
  (`LangVarRef::isGlobalRef()`, which makes `loadObj()`'s choice). These are
  the places [#706]'s checks are made:
  - a change: in `assignValue()`; in `evaluateCall()`, for a call on what a
    global holds that changes it (the `change` that `checkContextChange()`
    gets, which counts a parser's `finish`); and in
    `LangTerm::checkSendChange()`, for a send to a parser, not to a stream;
  - a take: in `LangVarRef::evaluate()`, of a value that `ctxHeldRef()`
    counts, unless it is read in place (`ctxValueInPlace`), and in
    `evaluateCall()`, of such a value that a call on what a global holds
    gives.
- `evaluateCall()` and `compileForIter()` note the functions and iterators a
  block calls (`CodeBlock::calls`), and `evaluateParse()` the types it parses
  (`CodeBlock::parses`).

## Restartable types

`Compiler::restartTypes` holds the types that restartable parsers parse:

- `LangTerm::evaluateNew()`: the type of every `new parser`.
- `LangTerm::evaluateParse()`: the type of a parse with an item after the
  first that runs other code. `Compiler::otherCodeRuns` counts the calls of
  `endReachableSetups()`, which [#706] makes wherever other code runs: a call
  of a function, a parse, a send to a parser, a `finish`, a user iterator's
  loop and a yield. `evaluateCall()` counts a call that changes what it is
  called on, such as `pop_tail`, too. An item whose evaluation changes the
  count runs code.

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

It then goes over the blocks in the order `compileByteCode()` compiled them,
functions and iterators, productions, tokens, preeof blocks, and reports the
first `GlobalUse` of the first block that is reached
(`Compiler::reportGlobalUses()`), so the error is the same one every time.

## Errors

The error is at the global's name:

- a change: `cannot change G, a global, in an action of a restartable parser: keep the data in a context`;
- a take of the global itself, of a field of it, and of what a call on it
  gives: `cannot take G, a global, out in ...`,
  `cannot take L out of G, a global, in ...` and
  `cannot take what find gives out of G, a global, in ...`;
- in a function or an iterator `f`, the error says
  `in f, which an action of a restartable parser runs` in place of
  `in an action of a restartable parser`.

## Choices

- Restartable parsers only, not every parser: the maintainer's brief. Ragel's
  parser and the manual's lexing examples change globals in parses that
  aren't restartable.
- At compile time, as [#706]'s check is, once all the code is compiled: what
  a parser runs is known only then, as for `checkRefCopies()`.
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

## Limits

- A tree that a global holds, whose attribute holds a list, map or struct,
  lets an action change the global's data through a copy of the tree. A
  restartable parser's action can read the tree into a variable and push
  onto the attribute's list, and backing up over the push after the program
  popped the list segfaults. This is [#720]'s fault, for a global.
- Two parsers that the program drives, given one context object ([#718]).

## Tests

In `test/colm.d`. `globchg1`–`globchg16` (changes) and
`globtake1`–`globtake4` (takes) are compile errors, one each, and say in
their first lines what they show. `globchg1`, `globchg2` and `globchg9` are
[#687]'s, [#683]'s and [#368]'s programs, through a global. `globchg3` is a
parse whose second item calls a function, and `globchg16` one whose second
item pops the list. `globread1` runs what stays: reads in place and a send
to a stream in a restartable parser, and a change undone in a parse whose
only item that runs code is its first. `nilset2` and `undomap10` became
parses that aren't restartable, and `sendown1`–`sendown4` keep their parsers
in a context.

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#683]: https://github.com/adrian-thurston/colm-suite/issues/683
[#687]: https://github.com/adrian-thurston/colm-suite/issues/687
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#720]: https://github.com/adrian-thurston/colm-suite/issues/720
