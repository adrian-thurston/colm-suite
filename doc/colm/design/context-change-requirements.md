# Changing a context object from outside its parse: requirements

For [#706]. This says what must hold of a context object while a parser it
was given can back up, and what colm checks. The
[design](context-change-design.md) says how the compiler checks it.

## Terms

- A *context* is a struct that a def or a token is defined in. A *context
  object* is an object of one, given to a parse or to a `new parser`.
- The parse's *own actions* are its token and reduction actions and the
  functions they call. They change the object through the context's
  variables, by name, and in a token action through `this`.
- *Outside* code is any other code with a reference to the object: the
  main program between two sends, a function a `parse` item calls, another
  parse's actions.

## Why

1. Backing up over an action puts back what the action changed, assuming the
   data is as the parse's actions left it. The undo of a list's push takes
   the value at that end off; the undo of a map's `remove` puts its element
   back without looking; the undo of a write puts back the old value. Since
   [#707] the undo checks nothing. A change from outside, made while the
   parser can still back up over the action, is lost, corrupts the list or
   map, or crashes the program.

## Requirements

2. A program that could change a context object from outside its parse's
   actions while a parser it was given can back up doesn't compile. The
   check is at compile time, and costs nothing when the program runs.
3. Compile time can't see when a parse ends, or which object a reference
   holds. So outside its parse's actions, a context object is changed only
   while it is set up: after it is made with `new`, before it is given to a
   parse, a parser, a function or another variable. It isn't changed after
   its parse either. A program makes a new object for each parse.
4. A change is an assignment to the object's variables, or to the fields of
   the structs it holds; a write into a tree it holds; a call that changes a
   list or a map it holds; and a send to, or a `finish` of, a parser or a
   stream it holds.
5. The setup existing programs do, writes and pushes after `new` and before
   the parse, keeps compiling. Context variables get no first values, and
   contexts no init block.
6. Outside code reads a context object anywhere, during its parse and after.
   It reads the lists, maps, structs and parsers the object holds in place:
   it can't take one out, where it could be changed through another
   reference. What goes into the object as it is set up is new, `nil`, or
   something the object holds already, so nothing outside holds it. Trees
   are values, copied on write, and are free to come and go.
7. Each refusal is a compile error at the reference it is about, saying what
   can't be done.

## Out of scope

- Two parsers that the program drives, given the same object ([#718]):
  compile time can't tell. The manual says to give each parser its own
  object, except a parse that a parser's actions start and a parser they
  send to.
- Globals that a parse's actions change and the program changes too: the
  companion rule, that an incremental parser keeps its data in a context,
  is [#714], with [#683] and [#687] its cases. [#368] is also met through a
  global accum parser.
- Host programs: exports give a host trees, never structs.

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#683]: https://github.com/adrian-thurston/colm-suite/issues/683
[#687]: https://github.com/adrian-thurston/colm-suite/issues/687
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#707]: https://github.com/adrian-thurston/colm-suite/pull/707
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
