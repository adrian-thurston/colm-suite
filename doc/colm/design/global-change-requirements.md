# Changing a global in a restartable parser: requirements

For [#714]. This says what must hold of a global while a parser whose
actions could change it can back up, and what colm checks. The
[design](global-change-design.md) says how the compiler checks it. It is the
companion of the
[ownership rule for context objects](context-ownership-requirements.md),
[#767].

## Terms

- A *restartable parser* is one the program can run code of its own in
  while the parser can still back up over its actions: a parser made with
  `new parser`, which the program sends to; a `parse` with an item after the
  first that runs code (a call of a function, a parse, a send to a parser, a
  `finish`, or a call that changes what it is called on); and a parse that
  an action of a restartable parser starts, since backing up over the action
  backs up the parse.
- What a parser *runs*: the actions of the productions of what it parses,
  the token actions and preeof blocks of the lex blocks it scans with, and
  the functions and iterators these call.

## Why

1. Backing up over an action puts back what the action changed, assuming the
   data is as the parse's actions left it. Between the sends to a restartable
   parser, the main program, a function a `parse` item calls, or another
   parser's actions can change a global that the parser's actions changed.
   The undo then works on data that isn't as it expects: it segfaults,
   writes into the wrong tree, corrupts a list or a map ([#687], [#683]), or
   takes out the wrong send ([#368]).
2. A parse that isn't restartable runs nothing else while it can back up, so
   its actions can change globals, and backing up undoes the changes.

## Requirements

3. In what a restartable parser runs, globals are read-only. A program that
   changes one there doesn't compile. The check is at compile time, and costs
   nothing when the program runs.
4. A change is an assignment to a global or to a field of a struct it holds;
   a write into a tree it holds; a call that changes a list, a map or a stream
   it holds; and a send to, or a `finish` of, a parser it holds.
5. That code can't take a list, map, struct, parser or stream out of a
   global, where it could be changed through another reference: put it into
   a variable, pass it to a function, return it or test it. A copy of a tree
   shares what its attributes, and those of the trees below it, hold, so a
   tree whose type can hold one of those there counts as one too. That code
   reads them in place.
6. It reads globals freely, and sends to a stream that a global holds: what
   goes to a stream isn't undone.
7. Each refusal is a compile error at the global, saying what can't be done
   and where, and that the data goes in a context.
8. Programs whose parsers aren't restartable compile as before: ragel's
   parser, `src/cgil`, `grammar/` and the manual's examples.

## Out of scope

- The undo of what the actions of a parse that isn't restartable do to
  globals, which still runs. [Its instructions](access-instructions.md) are
  those of any undone write.
- A tree that the program sends to a parser as itself, whose attribute holds
  a list, map or struct, through a global or not: the [rule for context
  objects](context-change-requirements.md) refuses it ([#729]).
- Two parsers given one context object: the
  [ownership check](context-ownership-design.md) refuses it ([#771]).
- Host programs: exports give a host trees, never structs.

[#368]: https://github.com/adrian-thurston/colm-suite/issues/368
[#683]: https://github.com/adrian-thurston/colm-suite/issues/683
[#687]: https://github.com/adrian-thurston/colm-suite/issues/687
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#729]: https://github.com/adrian-thurston/colm-suite/issues/729
[#767]: https://github.com/adrian-thurston/colm-suite/issues/767
[#771]: https://github.com/adrian-thurston/colm-suite/issues/771
