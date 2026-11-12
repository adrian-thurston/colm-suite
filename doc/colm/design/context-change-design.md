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
  to a parser, a parser's `finish`, a user iterator's loop.

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

## Limits

- Two parsers that the program drives, given one object, change it under
  each other. Only a parser that owns its context could rule that out.
- A list, map or struct that a parse's actions take out of their context
  into a global, or put into it from one, can be changed through the global.
  That is for the companion rule on actions and globals, which isn't filed.

## Tests

In `test/colm.d`. `ctxset1` runs what a setup may do, and reads the object
in place after its parse. `ctxchg1`–`ctxchg14` (changes),
`ctxtake1`–`ctxtake8` (references out, and what goes in) and `undoctx4` are
compile errors, one each, and say in their first lines what they show.
`ctxchg1` and `ctxchg2` are [#706]'s programs, and `ctxchg4` is [#368]'s
form. Most include `ctxrule.lmi`, a context to change.

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#550]: https://github.com/adrian-thurston/colm-suite/issues/550
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
