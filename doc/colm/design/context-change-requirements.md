# Changing a context object from outside its parse: requirements

For [#706], [#720] and [#729]. This says what must hold of a context object
while a parser it was given can back up, and what colm checks. The
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
   something the object holds already, so nothing outside holds it.
7. A copy of a tree shares the list, map, parser, stream or struct that an
   attribute of it, or of a tree below it, holds, so a tree whose type can
   hold one there counts as one too ([#720]). Outside code reads its
   attributes in place, and any use of the tree itself as a value, printing
   and matching it included, takes it out. The object gets such a tree only
   from its parse's actions: as it is set up, only `nil` or what the object
   holds goes in, since a `cons` or a `parse` can put into the tree what is
   held elsewhere. Other trees are values, copied on write, and are free to
   come and go.
8. A tree goes into a parse as itself, not as a copy, when it is sent with
   `send_tree`, is an item of a `parse_tree`, or is pushed onto a parser's
   input. A tree whose type can hold one of those values doesn't go in so
   ([#729]): the parse's trees would share what its attributes hold with the
   program, which could change it while the parser can back up. It goes as
   its text. A token action pushes such a tree onto its own parser's input,
   which is the parse's own data.
9. Each refusal is a compile error at the reference it is about, saying what
   can't be done.
8. A context object is held by one parser at a time ([#718]): two parsers
   given one object would change it under each other. Compile time can't
   tell which object a reference holds, so this is checked when the program
   runs. Giving a parser an object that another parser holds stops the
   program. A parser made with `new` holds its object to the end of the
   program, and a parse outside any action to its end. A parse that an
   action starts, with no item after the first that runs code, is backed up
   with the action: what it is given is held for the parser the action
   belongs to, and every such parse in that parser's actions can be given
   it. A parse with an item after the first that runs code holds what it is
   given for itself, and can't be given an object that the parser holds.

## Out of scope

- Globals that a parse's actions change and the program changes too: the
  companion rule, [#714], makes globals read-only in what a restartable
  parser runs, so that such a parser keeps the data it changes in a context.
  Its [requirements](global-change-requirements.md) cover [#683] and [#687],
  and [#368] met through a global accum parser.
- A tree that the program reads with `P->tree`, or gets from `finish()`,
  once the parser's input has ended: it shares what the parse's actions put
  into its attributes, and a send after the end makes the parser back up
  over them ([#439]).
- A cast to a tree type that can't hold a list, map, parser, stream or
  struct hides one from requirements 7 and 8, which go by type ([#755]).
- Host programs: exports give a host trees, never structs.

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#439]: https://github.com/adrian-thurston/colm-suite/issues/439
[#683]: https://github.com/adrian-thurston/colm-suite/issues/683
[#687]: https://github.com/adrian-thurston/colm-suite/issues/687
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#707]: https://github.com/adrian-thurston/colm-suite/pull/707
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#720]: https://github.com/adrian-thurston/colm-suite/issues/720
[#729]: https://github.com/adrian-thurston/colm-suite/issues/729
[#755]: https://github.com/adrian-thurston/colm-suite/issues/755
