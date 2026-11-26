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
  name that says so (`countOtherCode()`). `ctxValueInPlace` lost its
  context use with [#773]: `evaluateSetupValue()` set it for a value read
  through the root it goes into, which no read refusal is left to need. Its
  other uses, a send's target and a `for` loop's value (`forInPlace()`),
  are [#714]'s, for a take out of a global, so it is
  `Compiler::globalInPlace` now.
- **The parts of [#720]'s rule that loans replace** (PR #749), gone with
  [#773]: `checkContextValue()`'s refusal of any use as a value of a
  reference value read through a context object, a tree that can hold one
  among them: put into a variable, given to a function, tested, printed,
  matched, its text taken with `$`, or looped over. What stays of
  `checkContextValue()` is a loan's escape, checked where a value is
  stored ([Loans](#loans)). `sharesRef()` and `Compiler::treeHoldsRef()`
  stay: [#714] and [#729] use them, and so do loans.
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
- **Documents.** The requirements and the design of [#706]'s and [#720]'s
  rules for changing a context object are deleted ([#773]), once their
  section on trees into a parse moved here
  ([17](#trees-into-a-parse-17)). The requirements link to them as they were
  at a commit before.
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
- `checkTreeIntoParse()` ([17](#trees-into-a-parse-17)).
- `checkContextValue()`, as the check of a store: a reference value read out
  of a context object can be kept only in a local variable, or in the object
  it came from ([Loans](#loans)).
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
running context is lent, or while a parameter is lent: the caller could
have given it what the body uses. A local of the function or the iterator,
lent, is safe: the body can't reach it.

    cannot yield while the context object is lent (12:7): the loop's body
    could use it
    cannot yield while O is lent (12:7): the loop's body could use it

A function that an iterator calls can yield too, for the iterator. So a
call made while the running context is lent, or while a parameter is lent,
is noted the same way. That is a lend to a parser, to a loop, or to a parse
whose item makes the call: the yield would run the body between two of the
parse's sends. Each block notes whether it yields
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

A parser carries its context object too. An argument after a context
argument can take that argument's object away, as
`f( C, new parser<ctx::start>( C ) )` does, directly or through a function
that returns the parser. Then the function gets the object twice, as its
parameter and in its parser. So once the arguments are made
(`checkContextArgsKept()`, after `evaluateArgs()`, and before an
iterator's arguments are lent to its loop), the root of each context
argument must not have moved:

    cannot give f C: another argument takes it with the parser made at
    12:7

A loan on a context object is an argument too ([Loans](#loans)). Given with
an argument of a context type that has the loan's root, as `f( C, C->L )`
and `f( X, C )` for a loan `X` on `C` are, the function could lend the one
and use the other, so `checkContextArgs()` refuses it with the same error.
A loan on the running context is noted in `CodeBlock::contextArgs` as an
argument the running context reaches is, and refused for a function or an
iterator that names the context.

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

A store through a loan ([Loans](#loans)) reaches what the object holds, so it
is a store through a context reference too, on the loan's root, as
`X->L = G` is for a loan `X` on `C`. `setCtxStore()` notes it when the target
is qualified and starts at a local whose loan has a context root, and the
error names the root, or says `the context object`. Read through the same
root now means a value whose loan is on just that root, and *held*: what
holds only what its roots hold, or new objects. A function's result isn't
held, since the function could give a global's list; the tree of a parse
given the object is, since the parse's own code takes nothing out of a
global (13), and the store rule above keeps what goes into a loan the
object's.

## Trees into a parse (17)

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

## Loans

A *loan* is a value of a type `heldRef()` counts that came out of a context
object: a reference value, or a tree that can hold one. The compiler records
it on the local that holds it, as an entry of its own in `Compiler::owns`
(`Own::Loan`), beside any other the variable has: `findOwn()` skips it and
`findLoan()` finds it, `addOwn()` and `addLoan()` each replace only their
own kind, and `removeOwn()`, for a variable given another value, takes both.
The entry names the roots it is a loan on, each a `Compiler::LoanRoot` that
says whether its loan has ended, how and where, and says whether the value
is held ([What goes in](#what-goes-in-12)).

### What a value is a loan on

`valueLoan()` and `varRefLoan()` find it from the expression's form and the
ownership state, after the expression is evaluated, and evaluate nothing
(14):

- a reference read through a context reference (`firstCtxPart` set): its
  root;
- a reference that starts at a local holding a loan: that loan's roots;
- inside a function, a reference that starts at a parameter of a reference
  type other than a context: the parameter, a *parameter root*, which is
  never lent and only feeds `keeps` and `fills` below;
- the tree of a finished parser variable `P`, read as `P->tree`, and what
  its `finish()` or `eof()` gives: the root `P` was lent, which `P`'s
  `Finished` entry keeps (`Compiler::finishParser()`);
- a built-in call on any of these: what it is called on;
- a call of a function or an iterator: each argument's loan, and the root of
  each argument of a context type, not held;
- a parse given an object a root reaches, of a type `sharesRef()` counts:
  that root;
- a cast, a `match`, a search, a send's value and the `^` and `@` of a loan:
  its loan. The checks go by type, so a cast to a type that can't hold a
  reference value hides one ([#755]).

### Where loans are made, and end

A local (`UserLocalType` or `ParamValType`, not a tree loop's variable) given
a loan in an assignment holds it. So does a parse's capture, given the
object a root reaches; a `for` loop's variable, from its argument or, for a
user iterator, from what the iterator is given (added just after the body's
start state is taken, so `endScopeOwns()` drops it at the body's end); and a
pattern's variable in a `match` of a loan, which covers a `switch` too.

`Compiler::endLoans()` ends the loans on a root where it is lent to a parse,
a parser or a loop, moved, gone with a parser, or given another object (an
assignment to the root), and keeps where, for the error. A use of a local
whose loan has ended fails in `checkOwnedUse()`, which
`LangVarRef::preEvaluateRef()` now calls too, for a ref argument and a
`yield`:

    cannot use X: it is a loan on C, which was lent at 15:3
    cannot use X: it is a loan on C, which was moved at 15:3
    cannot use X: it is a loan on C, which went with the parser made at 15:3
    cannot use X: it is a loan on C, which was given another object at 15:3

A loan on the running context says `a loan on the context object`.

### Stores

A loan, or a reference value read out of an object, is kept only in a local
variable or in the object it came from (15). Every other store is
`checkContextValue()`'s, once its value is evaluated:

- an assignment to anything but such a local: a global, a qualified field, a
  context's variable named alone, a reduction's `lhs` or `r1`, a `ref`
  parameter, which writes into its caller's variable, and a tree loop's
  variable, which writes into the walked tree;
- a value argument of a built-in that changes what it is called on, such as
  `push_tail`, `insert`, `set` or `push`, and any argument of a function the
  host defines, which `evaluateCall()` tells `evaluateArgs()` of with
  `Compiler::argsGoIn`;
- an item and a field initializer of a `cons`, the attributes `make_token`
  is given and the children `make_tree` is given;
- a stream sent to a parser, which goes into its input as itself.

A store through a context reference, or through a loan, goes through
`evaluateSetupValue()` instead ([What goes in](#what-goes-in-12)). The error
is today's, at the value:

    cannot take L out of C, a context object
    cannot take what f gives out of C, a context object

For a loan that isn't read through `->`, it names the root, or says
`cannot take X out of the context object`. Every other use of a value is in
place, and fine while the loan lasts: printing it, sending it as text,
testing and comparing it, `!`, `$`, `$$` and `%`, `match`, a loop over it, a
send to it, a built-in call that doesn't change what it is called on, a
statement's dropped value, and `return`. Printing or matching `C->T` reads
through `C`, which can't be lent there, so [#720]'s refusal of those goes.

### Functions

Inside a function, what is stored of a parameter root marks the parameter
as kept (`ObjectField::keeps`), and what is put into what it reaches, unless
it is new, `nil` or held and got from that parameter alone, marks it as
filled (`ObjectField::fills`). A store into what a parameter reaches is one
whose target is qualified and starts at the parameter, or at a local whose
loan is on parameters and no context object. A `return` and a `yield` mark
nothing: the caller's result is a loan on what the arguments were.

A loan on a context object given to a function or an iterator the program
defines is noted (`CodeBlock::loanArgs`: the function, the parameter, where,
what and the root), in the block being compiled on its revert compile, or
in `rootCodeBlock`, which is compiled once. A parameter root given to
another function's parameter adds `refFlow()`s from its flags to that
parameter's. `checkRefCopies()` closes every flow, so
`Compiler::checkLoanArgs()`, after `checkLentCalls()`, sees the flags closed
over the calls, and reports the first noted loan given to a parameter that
keeps or fills:

    cannot give X, a loan on C, to f: f keeps its parameter L
    cannot give X, a loan on C, to f: f can put into its parameter Q what C
    doesn't hold

### Trees out of a parser (16)

`VarRefLookup::parserTree` is the part of a reference that reads the tree of
a parser of a type defined in a context. Where `checkOwnedUse()` is called,
`checkParserTree()` lets a tree of a type `sharesRef()` counts be read only
through a local parser variable `P` that is finished, and whose root hasn't
been lent, moved or given another object since:

    cannot read P->tree: P is lent a context object until P->finish(), and a
    T can hold a list, map, parser, stream or struct
    cannot read P->tree: it is a loan on C, which was lent at 15:3
    cannot read Q->tree: the parser has its context object for good, and a T
    can hold a list, map, parser, stream or struct

The last is a parser made with `new parser<T>( new ctx() )`, one in a field,
a parameter, or one that went with its object: it can take input after its
end ([#439]). Its `finish()` and `eof()` of such a type are refused the same
way, unless the tree goes nowhere, as in a statement of its own
(`Compiler::callDropped`), since a parser given its object for good must
still be finished:

    cannot use what Q->finish() gives: the parser has its context object
    for good, and a T can hold a list, map, parser, stream or struct

### Loans in branches and loops

`mergeOwns()` merges a loan only with the same variable's loan: the roots are
unioned, a root ended on either side has ended, and the value is held only
if it is on both sides. A `Finished` entry's root merges the same way. An
`if`'s own scope, which holds the variables its test's patterns declare, is
dropped after the merge, since the else side kept what the test gave them,
and `leaveLoop()` drops the loop's scope from the state its test left.

A loop's body can't make a loan in a variable from outside the loop, nor end
one: the next pass would start with the loan, which the body's statements
before the assignment weren't compiled for, or use it before the body ends
it. `checkLoopOwns()` refuses either at the loop:

    X takes a loan on C in the loop's body (14:5), and still holds it at its
    end
    X's loan on C ends in the loop's body (14:5), and the next pass could
    use it

## Globals in a context parse

`Compiler::checkRestartable()` walks from `restartTypes` to what restartable
parsers run, and refuses there every change of a global and every take out
of one. A second walk, the same way, starts from every type defined in a
context, and refuses there only what crosses the fence (13): a take out of a
global, and a change that puts a reference value into one, an assignment or
a call such as `push_tail` given one, or a stream sent to a parser it holds.
`GlobalUse` notes which changes put a reference value in. The errors are
#714's, saying `in an action of a parse given a context` in place of
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
  - A dropped `finish()` of a parser given its object for good is allowed: it
    gives no tree, and the parser has to be finished.
  - A loan made in a loop's body in a variable from outside it is refused,
    as a lend there is.
  - A `ref` parameter, a tree loop's variable and a host function's argument
    are stores: each puts the value where a local doesn't hold it.
  - A store through a loan is one into the object, even into the tree's own
    attribute, which writes only the local's copy: colm can't follow which
    of a loan tree's attributes still hold what the object holds. So
    `pointer1`'s `Input.Foo = Held`, through a parse's tree, is refused, as
    the maintainer accepted.
- What the parse's own code keeps of its object, in a local read from a
  context variable named alone or through `this`, while it lends the
  object, is left out ([#773]): the maintainer's answer. A rule for it needs
  more than loans: the parse's own code puts what its object holds into its
  trees and into new structs (c++.lm's `DD->lookupObj = lookupNs->tail`).
- What goes into a loan inside a function has its own summary,
  `ObjectField::fills`, beside `keeps`: the maintainer's answer. Requirement
  12 lets outside code put into what the object holds only new objects,
  `nil` or what it holds already, and a function given a loan is outside
  code.
- A value is held, and can go back into its object, only if it holds what
  its roots hold or new objects. Without that, what a function given a loan
  gives, which can be a global's list, would count as read through the
  root.
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
- The parse's own code can read what its object holds into a local, by a
  context variable's name or through `this`, lend the object to a new
  parser, and use the local between the sends. Nothing refuses it.

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
- **The cases.** [#771] added `ctxown1`–`ctxown42`, one refusal each, and
  the run cases `ctxlend1`–`ctxlend3`, the last [#757]'s programs.
  `ctxown28`–`ctxown42` share `ctxpush.lmi`. `argcount1` and `argcount2`
  give a call more arguments than it has parameters, which the argument
  check leaves for `evaluateArgs()` to report.
  `ctxhold5`'s runs became `ctxhold5`, `ctxhold10` and `ctxhold11` (refused),
  `ctxhold12` and `ctxhold13`.
- **Loans.** [#773] added `ctxloan1`–`ctxloan27`, one refusal each:
  - a loan used after its root is lent to a parse (1), to a parser (2), with
    a parse's capture (3) and `finish()`'s tree (4) as the loan, and after
    its root is moved (5), goes with a parser (6) or is given another object
    (7);
  - a loan stored in a global (8), a struct's field (9), a local's list
    (10), a `cons` (11), a host function's argument (12) and a caller's
    variable through a `ref` parameter (26);
  - a loan given to a function that keeps its parameter (13), or gives it to
    one that does (14), or puts a global's list into it (15), and outside
    code putting one into a loan (16);
  - `P->tree` while `P` is lent (17), `finish()` of a parser that has its
    object for good (18), and `P->tree` once its object is lent again (19);
  - a function given the object and a loan on it (20), and a loan on the
    running context given to a function that names the context (25);
  - a loan made (21) or ended (22) in a loop's body, a `for` loop's variable
    used after a lend (23), a `match`'s variable stored (24), and an ended
    loan given by `ref` (27).

  `ctxloan28` and `ctxloan29` run: loans read and changed after their
  parse, a function's result, a loan given to a function that keeps
  nothing, put back into its object, and taken on one side of an `if`. They
  share `ctxloan.lmi`. `ctxtake1`–`ctxtake5`, `ctxtake8` and `ctxtake15` run
  now, and `pointer1` is refused at its store through a parse's tree.

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
