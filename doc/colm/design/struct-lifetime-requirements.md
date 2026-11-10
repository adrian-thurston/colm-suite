# Struct lifetime and weak references: requirements

Part of [#663]. This says what must be true of struct lifetime and weak
references once #663 lands. It is the seed the design documents build on
([#667] the runtime model, [#668] the instructions, [#669] backing up,
[#670] weak references, [#671] the cycle check), not the design itself: it
says what, and leaves how to them.

## Terms

- A *struct* is any value the runtime keeps as a `struct colm_struct`: a
  user struct or context, a `list` or a `map`, a `list_el` or a `map_el`, a
  `parser`, an `_input`, a `stream`, and the program's globals.
- A *strong reference* keeps its struct alive. Every struct reference a
  program holds is strong, unless it is declared weak.
- A *weak reference* doesn't keep its struct alive. How one is declared is
  for [#670] to decide.

## Lifetime

1. A struct is freed as soon as the last strong reference to it goes, not at
   program exit. No freeing is deferred, apart from what backing up needs
   (7).
2. A strong reference is held wherever a struct can be: a struct field, a
   local, a parameter, a return value, a global, a list's value, a map's key
   or value, a value the VM holds part way through an expression, a def or a
   token attribute of a tree, and the references the runtime keeps for the
   program, such as a parser's to its context and its input, and those to
   `stdin`, `stdout`, `stderr` and `argv`.
3. A strong reference goes when its place is overwritten, when its local or
   parameter goes out of scope, however its function or action is left (a
   return, a reject or exit), when its element leaves its list or map, or
   when what holds it is freed.
4. Freeing a struct releases the strong references it holds, to trees and to
   other structs, which may free them in turn. Freeing a struct that leads to
   a great many others, such as a list of a million structs, must not
   exhaust the C stack, just as freeing a large tree doesn't.
5. A def or token attribute that holds a struct, and isn't declared weak,
   holds a strong reference: copying the tree, as copy-on-write does, adds
   one, and freeing the tree releases it. Trees keep their own `refs` scheme
   as it is.
6. At exit, every struct still allocated is freed, as today.

## Backing up

7. Backing up over an action leaves the structs as if the undone change had
   never run:
   - A struct whose last strong reference a change dropped is kept while the
     change can still be undone. Once it can't, the struct is freed if
     nothing else holds it.
   - A struct that only an undone change held is freed when the change is
     undone.
   - Undoing a write to a weak reference gives it back its old target, or nil
     if that target has been freed since. The undo record doesn't keep the
     old target alive.

## Weak references

8. A weak reference can be declared wherever a strong one can: a struct
   field, a local, a parameter, a return type, a global, a list's value, a
   map's value, and a def or token attribute of a tree. It can't be a map's
   key: a key that turned nil inside a map would break the map's order,
   which is why colm stops a write to the key of an element a map holds.
9. A weak reference can refer to any struct a strong reference can.
10. A weak reference doesn't keep its target alive.
11. Once its target is freed, a weak reference reads nil, never a dangling
    pointer. Reading or setting a field through it then stops the program,
    as it does through any nil struct.
12. A weak reference reads as its target: it can be used wherever a strong
    reference to its target's type can, and gives the target, or nil.
    Storing what it reads in a strong place makes a strong reference, and
    storing a strong reference in a weak place makes a weak one.
13. Copying a tree whose attribute holds a weak reference gives the copy a
    weak reference to the same target.

## Cycles

14. A program that compiles can't build a cycle of strong references. Colm
    rejects, at compile time, a struct type that can reach itself through
    strong references alone. Every strong reference counts: struct fields, a
    list's values, a map's keys and values, the runtime's own, such as a
    parser's to its context, and those that trees hold. A struct that can
    hold a tree (an `any` can be any tree) reaches every struct type that an
    attribute of that tree, or of a tree that can be below it, can hold. A
    weak reference on a path breaks it. Each cycle of types needs a weak
    reference in it: one elsewhere among the same types isn't enough.

    This rejects a struct type that holds its own type through a strong
    field, as a singly linked list does, or through a list, as a node with a
    `list` of its children does. In this repository that is
    `doc/colm/code/struct_route.lm` (`Next: stop`), and `lang_object` in
    `grammar/c++/c++.lm` and `test/colm.d/lookup1.lm`, which change when the
    check lands.
15. Apart from what 14 rejects, a program that compiles today still
    compiles, and prints the same output.

## Seeing lost structs

16. With `COLM_LEAK_CHECK` set, and always in a DEBUG build, the runtime
    reports the structs still allocated once exit has released the
    program's own references (its globals, `stdin`, `stdout`, `stderr` and
    `argv`), as `warning: lost structs: N`, the way it reports lost kids and
    trees. The test harness then fails a manual example that loses one, and
    a colm.d case that does, unless the run's `LOST` section has the line
    `lost structs: N`. The runtime still frees them, so valgrind stays clean.

## Validation

colm.d cases show each of these, with `COLM_LEAK_CHECK` and `--valgrind`
clean:

- A struct is freed as soon as its last strong reference goes: a weak
  reference to it reads nil right after.
- A struct that only a tree attribute holds stays alive while the tree does,
  and a weak attribute reads nil once its target is freed.
- A parent and child, the child linked back by a weak reference, are freed in
  full: no lost structs.
- Backing up keeps a struct that an undone action dropped, frees one that
  only the undone action held, and gives a weak reference back its old
  target, or nil.
- Colm rejects (a `COMP_ERR` case) a struct type that holds itself, two that
  hold each other, one that holds itself through a list, and one that holds
  itself through a tree attribute, and accepts each once a weak reference is
  on every cycle.

## Out of scope

- Trees keep their `refs` and downref scheme as it is, and cycles that run
  only through trees are not part of this.
- How a weak reference is declared, the runtime layout, the instructions,
  how backing up is done, and how the check works: [#667] to [#671].
- Host programs: exports give a host trees, never structs, so a host holds
  no struct references.

[#663]: https://github.com/adrian-thurston/colm-suite/issues/663
[#667]: https://github.com/adrian-thurston/colm-suite/issues/667
[#668]: https://github.com/adrian-thurston/colm-suite/issues/668
[#669]: https://github.com/adrian-thurston/colm-suite/issues/669
[#670]: https://github.com/adrian-thurston/colm-suite/issues/670
[#671]: https://github.com/adrian-thurston/colm-suite/issues/671
