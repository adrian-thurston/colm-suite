# Owning and lending a context object: design

How the compiler meets the [requirements](context-ownership-requirements.md).
The checks run in `src/colm/synthesis.cc` as code is generated, and in a
pass after all of it is compiled, as [#714]'s do. The run-time hold of
[#718] goes, and nothing takes its place when the program runs. The reverse
code doesn't change. A number in brackets, as (9), is a requirement's.

## Removing what came before (20)

The partial solutions come out whole, in the same change. Nothing of them
stays as dead code, or as a second check beside the new one.

- **[#706]'s setup windows and change rule** (PR #717):
  `Compiler::SetupWindow`, `setupWindows`, `findSetupWindow()`,
  `closeSetupWindow()`, `setupVar()`, `setupStoreVar`, `newParserContext`,
  the window the `AssignType` statement opens for `new`, and
  `checkContextChange()`, with its calls in `assignValue()`,
  `evaluateCall()` and `LangTerm::checkSendChange()`. `evaluateSetupValue()`
  keeps only [what goes in](#what-goes-in-12). `endReachableSetups()` keeps
  only its count of `otherCodeRuns`, which `restartTypes` needs, under a
  name that says so (`countOtherCode()`). `ctxValueInPlace` loses its
  context uses, the send target and what a change puts in, when [#773]
  turns `checkContextValue()` into the loan's escape: until then, its take
  refusal needs them. [#714]'s takes of a global still use it.
- **The parts of [#720]'s rule that loans replace** (PR #749):
  `checkContextValue()`'s refusal of a tree that can hold a reference value,
  read through a context object, used as a value: printed, matched, its text
  taken with `$`, or looped over. What stays of `checkContextValue()` is a
  loan's escape. `sharesRef()` and `Compiler::treeHoldsRef()` stay: [#714]
  and [#729] use them, and so do loans.
- **[#718]'s run-time hold** (PR #750): `IN_SET_PARSER_OWNER`,
  `IN_RELEASE_PARSER_CONTEXT`, `PARSER_NEW`, `PARSER_PARSE` and
  `PARSER_RESTART_PARSE`, the hold test in `IN_SET_PARSER_CONTEXT` (its stop
  on a nil context stays), `context_hold()`, `parser_owner()`,
  `pda_run->holding` and `pda_run->hold_owner` and their defaults in
  `colm_parser_new()`, the word `makeRuntimeData()` adds to a context's size
  in `sel_info`, and the message
  `giving a parser a context that another parser holds`. The five cases that
  80e61789 gave inner parsers objects of their own (`accumbt2`, `ctxset1`,
  `ntrejbt5`, `undofrag2`, `undofrag3`) stay as they are: the new check
  needs the same.
- **Tests that check only these.** `ctxholdmsg1`, and each `ctxchg` case that
  shows only how a window opens or ends, such as `ctxchg13` and
  `ctxchg15`–`ctxchg18`, unless its program shows something the new check
  refuses or allows. The rest are sorted as [Tests](#tests) says.
- **Documents.** `context-change-requirements.md` and
  `context-change-design.md` are deleted.
  `global-change-requirements.md` and `global-change-design.md` lose what
  they say of the hold and the windows.
  [`struct-lifetime-backing-up.md`](struct-lifetime-backing-up.md) loses
  its plan to make the hold's word a weak cell.
- **The manual.** `doc/colm/3_04_undo.adoc`'s "Setting up a context object",
  and its paragraphs on the hold; the paragraph in `3_06_send.adoc` on
  reading the object between sends; and the hold's bullet in
  `8_04_error_handling.adoc`. A section on lending takes their place.
- **Issues.** [#756] and [#757] are about the hold, and close with it.

## What stays

- `heldRef()`, `ctxHeldRef()`, `sharesRef()` and
  `Compiler::treeHoldsRef()`, which say what a reference value is.
- `checkTreeIntoParse()` (17).
- `checkContextValue()`'s refusal of a reference value read out of a context
  object into anything but a local variable, which becomes the loan's escape
  ([Loans](#loans)).
- [#714]'s `checkRestartable()`, which the
  [globals check](#globals-in-a-context-parse) widens.
- `checkContextGiven()`, `checkParserContext()`, and the refusal of a second
  context.

## Where a context type can be

A context type is the type of a local variable, a context's field, a
parameter and a function's return (4). `StructDef::isContext` is set in
`Namespace::declare()` when a def or a token is declared in the struct,
which `declarePass()` finishes before `resolvePass()` resolves any type. A
`context` block with neither is a plain struct. `TypeRef::resolveType()`
doesn't know where its type is used, so the checks go where each place's
type is resolved, in `src/colm/resolve.cc`:

- a global, in `Compiler::resolveParseTree()`'s loop over
  `globalObjectDef`, as `checkExports()` refuses an `export` of anything but
  a tree;
- a field of a struct that isn't a context, in its loop over `structEls`
  (which visits the struct of globals too, and the `list_el` and `map_el`
  structs a list or a map makes, whose fields are checked as below);
- a def's or a token's attribute (`UserFieldType`), in its loop over
  `langEls`;
- a list's value, in `resolveTypeListEl()`, and a map's key or value, in
  `resolveTypeMapEl()`, at the user's type, so the error has its place;
- the `T` of a `weak<T>`, in `resolveTypeWeak()`.

A `ref<T>` takes only a tree already. The error, at the place's type:

    G can't hold a ctx: a context object is held only by a local variable,
    a parameter, a return or a field of a context

## The ownership state

`Compiler::owns`, a list of `Compiler::Own`, holds an entry for each root
the code has done something to. A *root* is a local variable or a parameter
(an `ObjectField`), or the *running context*: the object the code's parse
was given, which `this` and a context's variables named alone reach
(`isInbuiltObject()` for `this`, `isStructRef()` for the names). An entry
says one of:

- **lent** to a parse, for its statement, or to a parser variable `P`, until
  `P->finish()`, with where;
- **moved**, with where, and whether to another place or with a parser;
- for a parser variable lent an object: its root, and whether it is
  **finished**;
- for a local that holds a loan: the roots it is a loan on, and for each one
  whether the loan has **ended**, and where;
- for a local of a reference type that has held only a new object: that it
  is **unique** (12).

A root with no entry is the code's to use. `CodeBlock::compile()` saves and
clears the state for each block, and a function or an iterator starts with
none: its parameters are lent to it by its caller.

### Uses

Every reference loads its root in `LangVarRef::loadObj()`: a read through
`->`, a value, the receiver of a call (`callOperation()`), the target of an
assignment (`assignValue()`) and of a send, and a capture. A check there,
`checkOwnedUse()`, refuses a root that is lent or moved, and a local whose
loan has ended:

    cannot use C: it is lent to P (12:7) until P->finish()
    cannot use C: it is lent to the parse (12:7) for its statement
    cannot use C: it is lent to each (12:7) until its loop ends
    cannot use C: it was moved at 12:7
    cannot use C: it went with the parser made at 12:7
    cannot use X: it is a loan on C, which was lent at 15:3

A use of the running context, through `this` or a context variable, says
`the context object` in place of `it`, as in
`cannot use L: the context object is lent to the parse (12:7) for its
statement`. The error is at the root's name. The receiver of a call is
checked before its arguments, though it is loaded after them.

`loadObj()` only checks. The state changes where a value is given: an
assignment, a return, a call's arguments, a parse's and a `new parser`'s
context. Some arguments are compiled twice, once into a vector that is
thrown away (`chooseTriterCall()`, the dry run of a ref argument, a loop's
arguments), so a change is made only by the compile that keeps its code.

### Moves (5)

An unqualified local of a context type that is the right-hand side of an
assignment, or a `return`'s value, moves. The local's entry says moved,
and the place assigned to has the object. A parameter, `this`, a context
variable named alone or a field read through `->` can't move:

    cannot move C: the caller lends it for the call
    cannot move this: the parse lends it for the action
    cannot move Sub out of C: give the field another object in its place
    cannot move Sub out of the context object: give the field another object in its place

Giving a variable or a field another object ends what was recorded of the
old one: its entry goes, and loans on it end.

### Lent to a parse (6–8)

`LangTerm::evaluateParse()` evaluates the context argument, finds its root,
and records it lent from there until the parse's last fragment, after its
items. A parse given `C->Sub` lends `C`. A parse in an action given `this` or
a context variable lends the running context. Then:

- An item that uses the root fails in `checkOwnedUse()`.
- An item that calls a function that uses the running context is refused,
  as [the running context](#the-running-context) says.

After the parse, the root is the code's again, and the parse's tree is a
loan on it if its type can hold a reference value ([Loans](#loans)).

### Lent to a new parser (9–10)

`LangTerm::evaluateNew()` records its context argument's root as lent, and
leaves it to whatever takes the new parser:

- An assignment to an unqualified local `P` of a parser type: `P`'s entry
  names the root, not finished, and the root is lent to `P`.
- Anything else, as in `C->Inner = new parser<inner::x>( new inner() )`:
  the object goes with the parser for good. If it came from a root, the
  root moved, which is refused for a parameter, `this`, a context variable
  or a field (5).

Then, on `P`:

- `P->finish()` or `P->eof()` (`ObjectMethod::ParseFinish`, in
  `evaluateCall()`) marks `P` finished, and the root is the code's again. So
  does a send to `P` that ends with `eos`, which sends the end of the input
  as `finish()` does.
- A send, a `send_tree`, a second `finish()`, or `P->gets()` on a finished
  `P` is refused:

        cannot send to P: it was finished at 14:1
        cannot finish P: it was finished at 14:1
        cannot get the input of P: it was finished at 14:1

  `P->gets()` is refused before the `finish()` too: pushes onto the input
  it gives are sends the state can't follow.

        cannot get the input of P: it is lent a context object until P->finish()
- `P->tree` and `P->error` are read through `P` as before. `P->tree` of a
  type that can hold a reference value is refused until `P` is finished
  (16). Reading a tree before the input has ended already stops the program
  (`IN_GET_PARSER_MEM_R`), unless the parse failed or is a `parse_stop`.
- Any other use of `P` as a value, in `LangVarRef::evaluate()`, moves the
  root with the parser for good. So does a send's value, which is the
  parser, when it is kept, as in `Q = send P "a"`. Only a send that is a
  whole statement leaves `P` where it is. A finished `P` can't be used as a
  value at all: a function given it could send to it again ([#439]). Nor
  can a `P` lent what a parameter, `this`, a context variable or a field
  holds, since that can't go with it (10):

        cannot use P as a value: it was finished at 14:1
        cannot use P as a value: what it is lent (12:7) can't go with it
- Giving `P` another parser, and the end of the statement list `P` is
  declared in (`compileStmtList()`), end the lend: nothing can send to the
  old parser.

### The running context

While the running context is lent, in a parse's items or between
`P = new parser( Sub )` and `P->finish()` in an action, the code can't use
`this` or the context's variables, which `checkOwnedUse()` refuses, and
can't call a function or an iterator that does. Such a call is noted in the
block (`CodeBlock::lentCalls`). Each block notes, from its revert compile as
`globalUses` are, whether it names a context variable or `this`
(`CodeBlock::namesContext`). The pass after compiling closes `namesContext`
over `CodeBlock::calls`, and reports the first noted call of a function or
iterator that names the context:

    cannot call f while the context object is lent (12:7): f uses the
    context's variables

A `yield` runs the body of the loop over the iterator, and in an action
that body can use the context's variables. So a yield is refused while the
running context is lent, or while a parameter is lent to a parser: the
caller could have given it what the body uses. A local of the function or
the iterator, lent, is safe: the body can't reach it.

    cannot yield while the context object is lent (12:7): the loop's body
    could use it
    cannot yield while O is lent (12:7): the loop's body could use it

A function that an iterator calls can yield too, for the iterator. So a
call made while the running context is lent, or while a parameter is lent
to a parser, is noted the same way. Each block notes whether it yields
(`CodeBlock::yields`), and the pass after compiling closes that over the
functions it calls, but not over the iterators, whose yields go to their own
loops. It refuses a noted call of a function that can yield:

    cannot call f while O is lent (12:7): f can yield to a loop's body,
    which could use it

Otherwise, outside code needs no such note: a function can reach a context
object only through its parameters, which the caller can't give it while
the object is lent.

### Arguments and iterators (11)

A context object given to a user function needs no state: the call ends
before anything else runs, and in the function the parameter can't move. An
iterator's argument is lent from the call to the end of its loop
(`compileForIter()`, around `compileForIterBody()`), since the loop's body
runs while the iterator is suspended.

But the function lends a parameter as if nothing else reached its object.
So at a call of a function or an iterator (`checkContextArgs()`), two
arguments of a context type can't have one root: `C` and `C`, `C` and
`C->Sub`, or `this` and a context variable. Nor can an argument whose root
is the running context go to a function or an iterator that names the
context: the function could lend its parameter and use the variable by
name. That call is noted (`CodeBlock::contextArgs`), and refused once
`namesContext` is closed over the calls:

    cannot give f two arguments that C reaches: it could lend one and use
    the other
    cannot give Sub to f: f uses the context's variables, which reach the
    same object

Only a context object can be an argument through a root: what else a
context object holds can't be taken out of it until loans ([#773]).

### Branches and loops

Nothing saves compiler state across a branch today, so this adds it:

- **if and else** (`IfType`, which `else if`, `switch` and `require` compile
  to): the state is copied before the then-list, kept after it, put back for
  the else part, and the two are merged. A root is lent or moved after the
  `if` if it is on either side, and a parser variable finished on either
  side is finished after it. A `P` finished on one side only can't be sent
  to, and doesn't give its root back. Entries for variables declared in a
  branch go with the branch's statement list. The loader records the scope
  that each nested statement list declares its variables in on the
  statement that owns the list (`LangStmt::scope`), and `compileStmtList()`
  drops the entries of that scope's variables at the list's end.
- **Loops** (`compileLoopBody()`, and a `while` loop's condition): at the
  end of the body, what the body lent, moved or finished of what is outside
  the loop must be as it was at the start, or the next pass would use what
  the last one lent or moved. What the body gives back is safe on the next
  pass. The error is at the loop:

        C is lent in the loop's body (14:5), and isn't back by its end
        C is moved in the loop's body (14:5), and isn't given another object by its end
        P is finished in the loop's body (14:5), and the next pass can't send to it

  A loan that ended in the body is checked the same way.

  What the body gives back doesn't count after the loop, which can end
  before the body runs, or at a `break`. A loop leaves the state its test
  left (a `for` loop, the state its body starts from), merged with the
  state at each `break` (`Compiler::breakOwns`, kept as `breakJumps` is),
  less what the loop's own variables held (`leaveLoop()`).

`return`, `break` and `exit` don't end a path. A branch that ends with one
still merges, and the state at a `break` goes to the end of its loop too.

## What goes in (12)

`evaluateSetupValue()` loses its window test, and checks every store through
a context reference (`firstCtxPart` set: a qualification through `->` on a
context value, other than `this`): an assignment's right-hand side, and the
arguments of a call that changes what it is called on, such as `push_tail`,
`insert` or `set`. A reference value going in must be `new`, `nil`, read
through the same root, or a unique local, which then moves:

    only a new object, nil, what C holds or a variable used only through ->
    since its new can go into C

The unique state is the setup window as it was, for a local of any reference
type: the assignment of a `new` term opens it, and it ends at any use of the
local as a value other than its move into an object, or a store into it of
anything that couldn't go into an object. A tree that can hold a reference
value is never unique: what `cons` and `parse` make can hold what is held
elsewhere ([#720]).

The parse's own code, through the context's names and `this`, isn't checked
here. What it puts in comes from new objects, the object itself, its locals
and globals, and the [globals check](#globals-in-a-context-parse) refuses
the last.

## Loans

A loan entry, on a local, names the roots it is a loan on (14):

- **Made** by an assignment to an unqualified local of a reference value read
  through a context reference, or given by a call on what an object holds,
  as `X = C->L` and `R = C->M->find( K )`, which
  `checkContextValue()` refuses today; by the capture of a parse lent `C`, of
  a type that can hold a reference value; by `P->tree` and `P->finish()` once
  `P` is finished; and from a loan, by an assignment, a field or attribute
  read, a `for` loop's variable, and what a call given it returns.
- **Ended** where its root is lent, moved, or given another object: the
  entry keeps where, for the error.
- **Escapes**: a loan, or a reference value read out of an object, stored
  anywhere but a local or the object it came from, is refused as today:
  `cannot take L out of C, a context object`. A loan given to a user
  function or iterator is noted (`CodeBlock::loanArgs`: the function, the
  parameter, where).

Inside any function, each parameter of a reference type is a root of the
same kind. Storing it, or a value got from it, anywhere but a local marks
the parameter as kept (`ObjectField::keeps`), as does giving it to a
parameter that keeps. A `return` doesn't: the caller's result is a loan on
what the arguments were. The pass after compiling finds `keeps` with a
fixpoint over the calls, as `checkRefCopies()` does `refWrite`, and reports
the first noted loan given to a parameter that keeps:

    cannot give X, a loan on C, to f: f keeps its parameter L

Using a loan in place, printing it, matching it or looping over it, is a
use, and is fine while the loan lasts. Printing or matching `C->T` reads
through `C`, which can't be lent there, so [#720]'s refusal of those goes.

## Globals in a context parse

`Compiler::checkRestartable()` walks from `restartTypes` to what restartable
parsers run, and refuses there every change of a global and every take out
of one. A second walk, the same way, starts from every type defined in a
context, and refuses there only what crosses the fence (13): a take out of a
global, and a change that puts a reference value into one, an assignment or
a call such as `push_tail` given one. `GlobalUse` notes which changes put a
reference value in. The errors are #714's, saying
`in an action of a parse given a context` in place of
`in an action of a restartable parser`.

## Choices

- At compile time, nothing at run time, as [#706] asked: the reason for this
  design. The hold was the one run-time check, for want of knowing which
  object a reference holds.
- `finish()` ends a `new parser`'s lend, and the parser takes no more input:
  the maintainer's answer.
- No context-typed globals: the maintainer's answer. Code that a global
  reaches can't be followed through a function.
- No reads while lent: the maintainer's answer. Every use of a lent root is
  refused alike.
- Loans in this change: the maintainer's answer. They close the tree that
  carries references out of a parse, and let the program read into a
  variable what it could only read in place.
- The partial solutions come out in the same change, not beside it: the
  maintainer's answer. They add code that the new check makes unneeded.
- Lending part of an object lends all of it: an object can hold, in one
  field, what another holds, as `C->Other = C->Sub->L` puts it there.
- Kept simple at a small cost:
  - `return`, `break` and `exit` don't end a path.
  - A parse's first item can't use the object, though nothing has been sent
    when it runs.
  - A function argument isn't lent: only the parameter can't move, and two
    arguments can't reach one object.
  - A loan given to a function that keeps anything got from it is refused,
    even if the function keeps it in the same object.
- Not taken: a parser made with its context, owning it: a language change,
  with no way to give one object to one parse after another.
- Not taken: lifetime annotations. The checks stay in one function, with a
  summary of each parameter.

## Limits

- The checks go by type: a cast to a tree type that can't hold a reference
  value hides one ([#755]).
- A parser that was given its object for good takes input after its end
  ([#439]). Nothing outside can reach its object, and it gives no tree that
  could.

## Tests

In `test/colm.d`.

- **The hold.** `ctxhold1`–`ctxhold5`, `ctxhold7` and `ctxhold9` become
  `COMP_ERR` cases: the second lend is a use of a lent root, `ctxhold4`'s
  item uses the running context, and `ctxhold7` and `ctxhold9` have a
  context-typed global. `ctxholdmsg1` goes with its message. `ctxhold6` and
  `ctxhold8` still run.
- **Setup.** `ctxchg1`–`ctxchg18` are sorted again. A change while the object
  is lent stays an error, with the new message. A change after a parse, in a
  function or in a loop's body compiles, and moves to a run case.
- **Takes.** `ctxtake1`–`ctxtake21` are sorted again in the same way. Into a
  variable is a loan, and stays an error only where it escapes or is used
  after a lend.
- **Reads between sends.** `ctxset2`, `loopchg5`, `loopchg6` and
  `structsplit3` read the object while a parser holds it, and are rewritten
  or become errors. `loopchg5` reaches its stops through a global's list or
  map in a parse that isn't restartable. `loopchg6`'s stop, backing up over
  a change while a loop is over the list, can't be reached by a program
  that compiles, and `loopchg6` is refused at its loop. The stop stays in
  the runtime.
- **Context types.** `ctxset1` drops its context-typed global, and
  `ctxchg3`, `ctxchg11`, `ctxchg14` and `ctxchg18`, whose globals hold a
  context object, are refused at the global. `weak11` drops its
  `weak<ctx>`.
- **New cases** for each refusal: a use while lent, to a parse and to a
  parser; a use after a move, and after a parser went with it; a move out
  of a parameter, `this` and a field; a send and a second `finish()` after
  `finish()`; `P->gets()`; a branch that finishes on one side; a lend in a
  loop's body; an item's call of a function that names the context; each
  place a context type can't be; what goes in; a loan used after a lend; a
  loan's escape, and to a function that keeps it; and each widened globals
  refusal. A run case each for what is new: a change and a second parse
  after a parse, an object back after `finish()`, a unique local moved in,
  and a loan read after its parse.
- **The cases.** [#771] added `ctxown1`–`ctxown38`, one refusal each, and
  the run cases `ctxlend1`–`ctxlend3`, the last [#757]'s programs.
  `ctxown28`–`ctxown38` share `ctxpush.lmi`.
  `ctxhold5`'s runs became `ctxhold5`, `ctxhold10` and `ctxhold11` (refused),
  `ctxhold12` and `ctxhold13`.

[#439]: https://github.com/adrian-thurston/colm-suite/issues/439
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#720]: https://github.com/adrian-thurston/colm-suite/issues/720
[#729]: https://github.com/adrian-thurston/colm-suite/issues/729
[#755]: https://github.com/adrian-thurston/colm-suite/issues/755
[#756]: https://github.com/adrian-thurston/colm-suite/issues/756
[#757]: https://github.com/adrian-thurston/colm-suite/issues/757
[#771]: https://github.com/adrian-thurston/colm-suite/issues/771
[#773]: https://github.com/adrian-thurston/colm-suite/issues/773
