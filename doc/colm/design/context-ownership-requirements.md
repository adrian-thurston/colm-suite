# Owning and lending a context object: requirements

For [#767]. This says what must hold of a context object, and of what it
holds, so that colm can say of any program that compiles: while a parser can
back up over actions that changed a context object, no code outside that
parse can reach the object or anything it holds. It takes a small part of
Rust's ownership and borrowing: an object has one owner, a parser borrows it
for a time the code shows, and what is read out of it is a loan that ends
when it is lent again. There are no lifetime annotations, and nothing is
checked when the program runs.

It replaces the [rules for changing a context object](https://github.com/adrian-thurston/colm-suite/blob/e31a7d40255e49adfecea5850755617530b0ff6d/doc/colm/design/context-change-requirements.md)
of [#706] and [#720], and the run-time hold of [#718], which are removed
(20), and it settles [#756] and [#757]. [#729]'s rule for trees sent into a
parse stays. The [design](context-ownership-design.md) says how the compiler
checks it.

## Terms

- A *context object* is an object of a context: a struct that a def or a
  token is defined in. It is given to a parse or to a `new parser`.
- A *reference value* is one that more than one place can hold, so that a
  change through one is seen through the others: a list, a map, a struct (a
  context object included), a parser, a stream, an input or a `weak<T>`, and
  a tree of a type that can hold one of these in an attribute, its own or
  that of a tree below it ([#720]). An `int`, a `str` and any other tree are
  values, copied where they go.
- The *owner* of a context object is the one place that holds it: a local
  variable, a field of another context object, or a parser.
- To *move* an object is to put it into another place. The place it left
  can't be used until it is given another object.
- To *lend* an object to a parser is to give the parser its use for a time
  the code shows. While it is *lent*, the code that lent it can't use it.
  After, it has it back.
- The parse's *own code* is its token and reduction actions, the preeof
  blocks of the lex blocks it scans with, and the functions and iterators
  they call. It uses the object through the context's variables, by name,
  and in a token action through `this`.
- *Outside code* is any other code.
- A *loan* is a reference value that came out of a context object, held in a
  local variable.

## Why

1. Backing up over an action puts back what the action changed, assuming the
   data is as the parse's actions left it. The undo of a list's push takes
   the value at that end off; the undo of a map's `remove` puts its element
   back without looking; the undo of a write puts back the old value. A
   change from outside, made while the parser can still back up over the
   action, is lost, corrupts the list or map, or crashes the program.
2. The rules on main guess at two things compile time can't see there:
   which object a reference holds, and when a parser is done with an object.
   So outside code changes a context object only while it is set up, never
   after its parse, and reads it while a parser can still back up. Two
   parsers given one object are caught only when the program runs, by a hold
   that stops correct programs ([#757]) and accepts a parser nothing can run
   ([#756]). If an object has one owner at a time, and a parser's use of it
   ends at a place in the code, compile time sees both.

## Requirements

### The claim

3. A program doesn't compile if outside code could reach a context object,
   or a reference value the object holds, while a parser it is lent to can
   back up over its actions. The check is at compile time, and costs
   nothing when the program runs. The run-time hold of [#718] goes.

### One owner

4. A context object has one owner. A context type is the type of a local
   variable, a field of a context, a parameter (11) and a function's return,
   and of nothing else: not of a global, declared with `global` or `export`,
   a field of a struct that isn't a context, a list's value, a map's key or
   value, a def's or a token's attribute, or the `T` of a `weak<T>`. A
   struct is a context when a def or a token is defined in it.
5. Putting a context object that a local variable holds into another
   variable or into a field of a context object, or returning it, moves it.
   The local can't be used again until it is given another object. Nothing
   moves an object out of a field, a parameter or `this`. A field is given
   another object in its place.
6. Lending what an object holds lends the object. Giving a parser `C->Sub`
   lends `C`, which the program can't use until it has `C` back: a context
   object can hold what another of its fields holds, as
   `C->Other = C->Sub->L` puts it there.

### Lent to a parse

7. A `parse` is lent its context object for its statement. The parse's
   items can't use the object. A parse that an action starts, given the
   object the action belongs to (`this`) or one that object holds, can't
   have an item that uses the context's variables by name, or calls a
   function or an iterator that does.
8. After the statement, the object is the program's again, to read, change
   and lend again. A context object can be given to one parse after another,
   and changed between them.

### Lent to a new parser

9. `new parser<T>( C )` lends `C` to the parser until the parser's
   `finish()`. Until then the program can't use `C` at all, to read it or to
   change it. After the `finish()`, the program has `C` back, and nothing is
   sent to the parser again, nor is it finished again.
10. The parser stays in the variable it is put into. The program sends to it,
    finishes it, and reads its `tree` and `error` through that variable, and
    uses it no other way. If the variable goes out of scope before the
    parser's `finish()`, the lend ends there. If the parser goes anywhere
    else, into another variable, a field, a list or a map, a function or a
    return, or is made where no variable holds it, `C` goes with it for
    good: a local can't be used again until it is given another object, and
    an object that a field, a parameter or `this` holds can't go (5).

### Functions

11. A parameter of a context type is lent for the call. The function reads
    and changes the object, and lends it to a parse, or to a new parser that
    it finishes before it returns. It doesn't move it. An iterator's
    parameter is lent for the whole loop, its body included.

### What a context object holds

12. Outside code holds a reference value that a context object holds only as
    a loan (14). What outside code puts into the object, or into what the
    object holds, is a new object, `nil`, what the object holds already, or
    a local variable that has held only a new object since it was given one,
    has been used since only through `->`, and has had only such values put
    into it. That variable then can't be used again: its object moved.
13. The parse's own code doesn't take a reference value out of a global, or
    put one into a global, in any parse given a context object, whether it
    is restartable or not. In what a restartable parser runs, [#714] already
    refuses both.

### Loans

14. A loan on a context object is:
    - a reference value that outside code reads out of the object into a
      local variable;
    - the tree of a parse lent the object, of a type that can hold a
      reference value;
    - the tree that a parser lent the object gives, with `tree` or
      `finish()`, after its `finish()`;
    - a reference value got from a loan: a field or an attribute read from
      it, a `for` loop's value over it, and what a call given it returns;
    - a reference value that the parse's own code reads out of its own
      object into a local variable, by a context variable's name or
      through `this`.
15. A loan stays in local variables. It can go back into the object it is a
    loan on, and to a function whose parameter keeps it, and what the
    function gets from it, in its own local variables. A function given a
    loan puts into it only what 12 lets outside code put into the object.
    A loan that the parse's own code reads out of its own object can go
    where that code puts what its object holds. It ends where its object is
    lent or moved, or the variable that holds the object is given another
    one. A use of it after that doesn't compile.
16. A parser doesn't give a tree of a type that can hold a reference value,
    with `tree` or `finish()`, while it is lent an object, nor ever if it
    was given its object for good (10), such as a parser made with
    `new parser<T>( new ctx() )`. Its actions could still change what the
    tree's attributes hold, and back up over the change.

### Trees into a parse

17. A tree of a type that can hold a reference value goes into a parse as
    its text, not as itself, as [#729] made it: not with `send_tree`, as an
    item of a `parse_tree`, or pushed onto a parser's input, except a token
    action's push onto its own parser's `input`.

### Errors and programs

18. Each refusal is a compile error at the use, naming the object and the
    place where it was lent or moved.
19. Programs in `grammar/`, `src/ragel/`, `src/cgil/` and the manual's
    examples compile and run as before.

### Removing what came before

20. The partial solutions on main are removed in the same change, not left
    beside the new check: [#706]'s setup windows and change rule, the parts
    of [#720]'s rule for trees that loans replace, and [#718]'s run-time
    hold, with the code, instructions and fields that only they use. So are
    the test cases that check only them, their text in the manual, and the
    design documents that describe them. Nothing of them stays as dead code,
    or as a second check of what the new one checks.

## Out of scope

- Globals that a restartable parser's actions change: [#714]'s
  [rule](global-change-requirements.md) stays as it is.
- A cast to a tree type that can't hold a reference value hides one ([#755]):
  the checks go by type.
- Host programs: exports give a host trees, never structs.
- A parser given no context. Its actions change only globals, which [#714]
  covers.
- Text sent to a parser after its end ([#439]), for a parser that isn't lent
  a context object.
- When a struct is freed ([#663]). Owning here is a compile-time rule about
  which code can reach an object, and frees nothing.
- What the parse's own code puts of its object into its trees, or into new
  structs, while it lends the object, which it can then reach through them
  between the sends. It needs a rule of its own: the parse's own code puts
  what its object holds there as it builds them. A local it reads the
  object into is a loan (14).

[#439]: https://github.com/adrian-thurston/colm-suite/issues/439
[#663]: https://github.com/adrian-thurston/colm-suite/issues/663
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#720]: https://github.com/adrian-thurston/colm-suite/issues/720
[#729]: https://github.com/adrian-thurston/colm-suite/issues/729
[#755]: https://github.com/adrian-thurston/colm-suite/issues/755
[#756]: https://github.com/adrian-thurston/colm-suite/issues/756
[#757]: https://github.com/adrian-thurston/colm-suite/issues/757
[#767]: https://github.com/adrian-thurston/colm-suite/issues/767
