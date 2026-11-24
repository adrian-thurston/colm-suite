# Struct lifetime: backing up

Part of [#663], for [#669]. This is how the parser's reverse code keeps the
structs that backing up needs, and what each `_BKT` instruction does with
them, to meet requirement 7 of the
[requirements](struct-lifetime-requirements.md). It uses the calls of
[#667]'s [runtime model](struct-lifetime-runtime.md), the instructions of
[#668]'s [design](struct-lifetime-instructions.md) and the cell of [#670]'s
[weak references](weak-reference-design.md). It also gives a weak reference
to the hold that [#718] added to a context object after those designs. A
number in brackets, as (7), is a requirement's.

## Before

- A write that the parser can back up over (`_WV`) records how to undo it,
  as a unit, in its parser's `pda_run->rcode_collect`. A write through a
  qualification starts its unit at the root, with an instruction that
  pushes the object again (`IN_LOAD_GLOBAL_BKT`, `IN_LOAD_CONTEXT_BKT`,
  `IN_LOAD_INPUT_BKT` or `IN_PTR_ACCESS_BKT`), adds one for each tree it
  loads on the way, and ends the unit with the set, which puts the old
  value back (`rcode_unit_term()`). A push, a pull and a list's or a map's
  change end their object's unit the same way. A send and a parse record a
  unit of their own, which names the parser.
- When an action ends, `colm_make_reverse_code()` moves its units, last
  first, into `pda_run->reverse_code`, as one block, and
  `colm_transfer_reverse_code()` marks the tree the action belongs to
  (`PF_HAS_RCODE`).
- Backing up over that tree takes the block off with
  `colm_pop_reverse_code()`, and runs it through `pcr_call()`: `_BKT`
  instructions only. A `_BKT` that runs uses up its record: nothing
  releases a block that has run.
- A block that never runs is released by `colm_rcode_downref_all()`, which
  runs `rcode_downref()` on it. Only `colm_pda_clear()` calls it, from
  `colm_parser_destroy()`, so the reverse code lives as long as its
  parser. A commit point releases none of it: `commit_reduce()`
  (`commit.c`) frees only a reducer's parse trees.
- A record holds a tree that it puts back, or that a send sent, with a
  reference of its own, which `rcode_downref()` releases. It holds no
  struct. `IN_SET_STRUCT_VAL_BKT`'s old value, `IN_PTR_ACCESS_BKT`'s
  pointer, and the parser of a send and of `IN_PARSE_INIT_BKT` are plain
  words, which `rcode_downref()` skips. That is safe only because no struct
  is freed before exit. `IN_LOAD_INPUT_BKT` counts its input with
  `colm_tree_upref()`, but no program has it: `input` is a pointer, read
  before the arrow that every write through it takes.
- An action's locals are released when the action ends, by `IN_PCR_RET`'s
  `release_locals()`, before the parser can back up over it. A production
  action that rejects is backed up over as soon as it ends.
- A pop and a `remove` in an action give their element to the reverse
  code, which puts it back when it runs, and frees it when it is released.

## The rule

A struct is freed as soon as its last strong reference goes ([#667], (1)).
Nothing waits for the parse to end. So whatever a `_BKT` uses when it runs
must be held until then by the reverse code itself. Trees already follow
this rule: a record counts each tree it names.

1. A record that keeps a struct holds a strong reference to it, from the
   `_WV` that records it until its `_BKT` runs or its block is released. It
   holds the old value it puts back, whose reference the `_WV` moves out of
   the place it changes. It also holds what the undo acts on that nothing
   else is sure to hold: the struct a write goes through, and the parser a
   send or a parse goes to. The `_WV` counts these for the record, beside
   the stack's reference.
2. A record holds no reference to what something else holds for as long as
   the record can run:
   - the global, which the program holds until exit;
   - a parse's context, which its parser holds from its setup until the
     parser is freed, and the parser is held while its reverse code runs,
     by the instruction that runs it;
   - an element the reverse code keeps, which is the record's alone and
     isn't counted ([#667]);
   - a map element still in its map, which `FN_VMAP_INSERT_BKT` and
     `FN_VMAP_SET_BKT` name: the map holds it, and the unit's root holds
     the map;
   - a weak slot's old cell, which counts nothing ([#670]);
   - the stream a send records: the input's buffer holds it, and the undo
     frees the buffer ([#667]).
3. A `_BKT` that runs takes over its record's references, and counts
   nothing for them: an old value goes back into its place, and a pointer
   or a parser onto the stack. It counts what it pushes that no record
   held: the global, a context, or a tree it splits.
4. A `_BKT` releases the value it takes out of a place, which the undone
   change put there, and what it pops and keeps nothing of, its object
   last, as [#668]'s rule has every instruction do. So a struct that only
   the undone change held is freed by the undo (7). A stream freed there
   is closed there, and a close that fails stops the program there
   ([#667]).
5. For a block that never runs, `rcode_downref()` releases each strong
   reference a record holds: a struct with `colm_struct_downref()`, a tree
   with `colm_tree_downref()`, as today, and a cell with
   `colm_weak_free()`.
6. Each unit that a `_WV` starts is ended by the write after it. A unit
   left open makes `colm_make_reverse_code()` read the last byte of the
   unit's last word as its length, and the reference its record holds is
   never released. A call of a built-in that records no reverse code, a
   stream's `close()` or `auto_trim()` or an input's `auto_trim()`, loads
   its object as for a read, so it starts no unit ([#762]).

## Why each hold is needed

In programs that compile today, each struct a record names would
otherwise be freed before the parser backs up:

- **The struct a write goes through.** In an action, `S: s = new s()` then
  `S->N = 1`: the unit's root, `IN_PTR_ACCESS_BKT`, names `S`. The end of
  the action releases the local, and nothing else holds `S`, so backing up
  would write `N` into freed memory.
- **An old value.** `G = new s()` in an action drops what `G` held. Only
  the record of `IN_SET_STRUCT_PTR_BKT` holds it then.
- **A struct that a later change drops.** `G->N = 1`, then `G = nil`: the
  second unit's old value is the struct the first unit writes into. Units
  are undone last first, so the second puts it back before the first
  pushes it. Each record holds it.
- **A parser.** No variable holds the parser of a parse in an action,
  `parse X: T[ ... ]`, once the parse ends ([#668]), but backing up over
  the action backs the parse up: `IN_PARSE_INIT_BKT` pushes the parser, and
  `IN_PARSE_FRAG_BKT` undoes the parse, before the sends to it are undone.
  A `new parser` that a local of the action holds, and that the action
  sends to, is released when the action ends.

## When the holds end

A record's references go when its `_BKT` runs, which takes them over, or
when its parser is freed, as `colm_parser_destroy()` clears the parser's
run. A commit point releases nothing, as for trees, and requirement 7 says
so. The maintainer chose this in planning #669, over releasing at a commit
point the reverse code that the parser can't run any more, which would
change when trees are freed too, and would need working out which blocks a
commit leaves dead. In practice, what the reverse code holds is released:

- for a `parse` outside any action, at the end of the parse, when its
  parser is freed ([#668]);
- for a `new parser`, when the program drops the parser;
- for a parse in an action, when the parser the action belongs to is
  freed, since that parser's reverse code holds it.

## Each instruction

### Roots

Each pushes the object that the rest of its unit acts on.

- `IN_LOAD_GLOBAL_WV` records nothing to hold. `IN_LOAD_GLOBAL_BKT` pushes
  `prg->global`, counted.
- `IN_LOAD_CONTEXT_WV` records nothing to hold. `IN_LOAD_CONTEXT_BKT`
  pushes `exec->parser->pda_run->context`, counted. That is the context
  the write went into: a unit goes into the reverse code of the parser
  whose action recorded it, and a parser's context is set once, as the
  parser is set up.
- `IN_PTR_ACCESS_WV` counts the struct it records with
  `colm_struct_upref()`. `IN_PTR_ACCESS_BKT` pushes the struct with the
  record's reference. `rcode_downref()` releases it.
- `IN_LOAD_INPUT_WV` counts the input it records with
  `colm_struct_upref()`. `IN_LOAD_INPUT_BKT` pushes the input with the
  record's reference, and calls no `colm_tree_upref()`. `rcode_downref()`
  releases it. No program has either instruction (above).

### Loads on the way

- `IN_GET_STRUCT_BKT` pops a struct and pushes the tree its field holds,
  split and counted, as today, then releases the struct.
- `IN_GET_FIELD_TREE_BKT` and `IN_GET_RHS_VAL_BKT` take and give trees, as
  today.

### Sets

- `IN_SET_STRUCT_PTR_WV` ([#668]) records `IN_SET_STRUCT_PTR_BKT`, a half
  for the field and a word for the old value, whose reference moves into
  the record, and ends the unit. `IN_SET_STRUCT_PTR_BKT` pops the struct,
  takes the field's value out, puts the old value in, then releases the
  value it took out, and the struct. `rcode_downref()` releases the old
  value.
- `FN_SET_STRUCT_WEAK_WV` ([#668]) records `IN_FN`,
  `FN_SET_STRUCT_WEAK_BKT`, a half for the field and a word for the old
  cell, and ends the unit. `FN_SET_STRUCT_WEAK_BKT` pops the struct, takes
  the field's cell out, puts the old cell in, frees the cell it took out
  with `colm_weak_free()`, then releases the struct. The old cell reads its
  target, or nil if the target has been freed since (7). `rcode_downref()`
  frees the old cell.
- `IN_SET_STRUCT_BKT`, for a tree in a struct's field, and
  `IN_SET_STRUCT_VAL_BKT`, for an `int` or a `bool`, do what they do
  today, then release the struct they pop. `rcode_downref()` still skips
  `IN_SET_STRUCT_VAL_BKT`'s word: with [#668] a struct is set through
  `IN_SET_STRUCT_PTR_WV`, so the word is an `int` or a `bool`.
- `IN_SET_FIELD_PTR_WV` and `FN_SET_FIELD_WEAK_WV` ([#668]) record the old
  pointer tree with `IN_SET_FIELD_TREE_BKT`, as a set of a tree attribute
  does. The pointer tree holds its struct (`AF_STRUCT_PTR`) or its cell
  (`AF_WEAK_PTR`), so the record holds them through it, and
  `IN_SET_FIELD_TREE_BKT` and `rcode_downref()` stay as they are. The
  pointer tree an undo takes out releases its struct, or frees its cell, as
  `tree_free_rec()` frees it ([#667], [#670]).
- `IN_SET_RHS_VAL_BKT`, `IN_SET_TOKEN_DATA_BKT` and `IN_RESTORE_LHS` take
  and give trees, as today.

### Inputs and streams

- `IN_INPUT_PULL_BKT`, `IN_STREAM_PULL_BKT`, `IN_INPUT_PUSH_BKT` and
  `IN_INPUT_PUSH_STREAM_BKT` pop the input or the stream their unit's root
  pushed, undo as today, then release it. `IN_INPUT_PUSH_STREAM_BKT`'s
  undo frees the input's buffer, which releases the stream it held
  ([#667]).

### Sends and parses

- `IN_SEND_TEXT_W`, `IN_SEND_TREE_W`, `IN_SEND_STREAM_W` and
  `IN_SEND_EOF_W`, where they record, count the parser for the record with
  `colm_struct_upref()`, and push the stack's reference back, as [#668]
  has them. Their `_BKT`s undo the send as today, then release the parser.
  `rcode_downref()` releases it, beside the tree that a text or a tree
  send holds. `IN_SEND_STREAM_BKT`'s stream isn't counted: the undo frees
  the input's buffer, which holds it ([#667]).
- `IN_PARSE_FRAG_W`, where it records `IN_PARSE_INIT_BKT` at the end of a
  parse, counts the parser for the record. `IN_PARSE_INIT_BKT` pushes the
  parser with the record's reference. `IN_PARSE_FRAG_BKT` pops it and
  pushes it back while the parser backs up, and releases it as it pops it
  at the end. `rcode_downref()` releases `IN_PARSE_INIT_BKT`'s parser.

### Lists and maps

Each pops the list or the map its unit's root pushed, and releases it last.

- `FN_VLIST_PUSH_TAIL_BKT` and `FN_VLIST_PUSH_HEAD_BKT`, and
  `FN_VMAP_INSERT_BKT` and `FN_VMAP_SET_BKT` of a key they added, free the
  element they take out, which releases its value and its key ([#667]).
- `FN_VLIST_POP_HEAD_WV`, `FN_VLIST_POP_TAIL_WV` and `FN_VMAP_REMOVE_WV`
  push the value, counted by `colm_value_get()` ([#668]), and give the
  element, which keeps its own reference to the value, to the record.
  Their `_BKT`s put the element back, and `rcode_downref()` frees it, as
  today. None of them calls `heap_unlink()` or `colm_struct_add()` any
  more ([#667]).
- `FN_VMAP_SET_WV` of a key the map has moves the found element's old
  value into the record. `FN_VMAP_SET_BKT` puts it back, and releases the
  value it takes out with `colm_value_release()`. `rcode_downref()`
  releases the old value with `colm_value_release()`, whatever the value
  type, where today it releases only a tree. A weak value is a cell, which
  the record keeps ([#670]).

### Locals and calls

None has a `_BKT`. A write to a local is never undone, so no local
instruction has a `_WV` ([#668]). A call's arguments and its return value
are on the VM stack and in the frame, never in the reverse code. A
function or an iterator that an action calls runs its revert code, whose
writes record units in the action's parser, as the action's own writes
do. Its locals are released when it returns, before the parser can back
up over the action.

## Requirement 7

- A struct whose last strong reference a change dropped is kept until the
  parser that ran the change is freed: the record holds it (rule 1) until
  then, or until the record runs and puts it back.
- A struct that only an undone change held is freed when the change is
  undone: the `_BKT` releases what it takes out (rule 4).
- Undoing a write to a weak reference gives it back its old target, or
  nil: the record keeps the old cell, which counts nothing, so it keeps no
  old target alive ([#670]).

## A context object's hold

[#718] gave each context object a slot past its fields (`context_hold()`),
which names the parser that holds the object, so that no parser with
another owner is given it while that parser can still back up. It also
gave each parser `pda_run->hold_owner`, the parser whose backing up backs
it up ([design](context-change-design.md)). Both are plain pointers, which
nothing counts. Once parsers are freed, the slot can name a freed parser:
a `parse`'s parser is freed at the end of the parse ([#668]), and the next
parse given the object follows the slot, in `IN_SET_PARSER_CONTEXT`.
`ctxhold6`, and `ctxhold8`'s `after` run, do that. The maintainer chose,
in planning #669, to settle it here, with a cell.

- The slot keeps a weak reference, a [#670] cell, to the parser it names.
  It reads nil once that parser is freed: no parser holds the object
  then, and any parser can be given it.
- So a `new parser` holds its object until it is freed, not to the end of
  the program, as [context requirement 10](context-change-requirements.md)
  and the [context-change design](context-change-design.md) say today.
  Once the program drops such a parser, another parser can be given its
  object, where today the program stops. A freed parser can't back up,
  and backing up is all that the hold guards against. Those lines change
  when this is built.
- `IN_SET_PARSER_CONTEXT` reads the slot's parser with `colm_weak_get()`,
  and names the owner in it with `colm_weak_set()`. Setting up a parser is
  never undone, so the cell moves from parser to parser.
- `Compiler::makeWeaks()` ([#670]) lists a context's slot, the word past
  its fields, in `weaks`, so freeing the object frees the cell.
- A strong reference in the slot would make a cycle: the object would hold
  its parser, which holds the object as its context, or holds, through
  its reverse code, the parse that was given it.
- `pda_run->hold_owner` stays a plain pointer. It names another parser
  only for a parse that an action starts (`IN_SET_PARSER_OWNER`,
  `IN_RELEASE_PARSER_CONTEXT`). No program can name such a parser, so,
  once the parse has ended, only the reverse code of the action's parser
  holds it (rule 1). That parser is the owner, or is held the same way by
  a parser nearer the owner. So the parse is freed with its owner at the
  latest, and `parser_owner()`, which runs only in program code, never
  reads a freed parser.
- [#757] is about which parser holds an object, not how the slot keeps it.

## Cycles

[#671]'s check has no edge for what a parser's reverse code holds, and
needs none:

- Only a `new parser` can be held by a struct. Nothing can name the parser
  of a `parse`, and only the VM stack and reverse code hold it.
- The actions of a `new parser`, and the functions, the iterators and the
  parses they run, can't change a global or take a reference out of one
  ([#714]). So their writes go through, and drop, only what the parser's
  context, its input and its trees lead to, which are the parser's edges
  in [#671]'s graph, or structs that the same code makes, which can hold
  only what that code reaches.

## Exit

Nothing changes beyond [#667]'s and [#670]'s exit. A parser that exit's
step 5 frees releases its reverse code while `prg->clearing_heap` is set,
so `colm_struct_downref()` does nothing, and `colm_weak_free()` frees a
cell without taking it off a list. An element the reverse code keeps is on
no heap list, and is freed only through its record. No reverse code runs
at exit.

## Public interface

No opcode beyond [#668]'s: `IN_SET_STRUCT_PTR_BKT` and
`FN_SET_STRUCT_WEAK_BKT` are its, and their operands are above. No
installed header or exported symbol changes beyond those that [#667],
[#668] and [#670] list. The cell's part of [#670]'s, `struct colm_weak`
and its five calls in `struct.h`, `weak_pool` in `program.h`, and `weaks`
and `weaks_len` in `pdarun.h`, comes with this design, ahead of the rest
(below).

## Rejected

- **Releasing at a commit point** the reverse code the parser can't run
  any more: the maintainer's choice above.
- **Weak references in the records** for what a unit acts on, skipping the
  unit when its struct has gone: once `colm_make_reverse_code()` has moved
  a unit into a block, the unit's length is gone, so a `_BKT` can't skip
  the rest of it. An old value must be held strongly anyway (7).
- **A list on each parser of the objects it holds**, for the context's
  hold: it is taken apart from both ends, as a cell is, so it would be a
  second weak mechanism.
- **Leaving the context's hold to an issue of its own**: the maintainer
  chose to settle it here, since no parser can be freed without it.
- **A strong reference in the context's slot**: a cycle (above).
- **Edges for the reverse code in [#671]'s graph**: they would add no
  cycle that the graph misses (above).

## Validation

When this is built, colm.d cases show, beside the requirements' and the
other designs' Validation, with `COLM_LEAK_CHECK` and `--valgrind` clean:

- An action's write through a local's struct, which the parser backs up
  over: the undo writes the old value back into a struct that only the
  record held.
- `G = new s()` in an action that the parser backs up over: `G` holds the
  old struct again, and the new one is freed by the undo, so a weak
  reference to it reads nil right after.
- `G->N = 1`, then `G = nil`, in one action that the parser backs up over:
  `G` and its `N` are as they were before the action.
- A struct that an action dropped, which the parser can still back up
  over, is kept until the parser is freed: a weak reference to it reads it
  until then, and nil after, even past a commit point.
- A send and a parse in an action, to a `new parser` that only a local of
  the action held, are undone as the parser backs up over the action, and
  the `new parser` is freed with the parser the action belongs to.
- A pop, a `remove`, and a `set` of a key the map has, in an action that
  the parser backs up over: the element or the old value is back. Where
  the parser never backs up, it frees them as it is freed.
- A weak field written in an action that the parser backs up over reads
  its old target, or nil when the target was freed in between ([#670]).
- A production action that stores a new struct in a global, then rejects:
  the struct is freed as the parser backs up over the action.
- Two parses given one context object, one after the other, the first's
  parser freed at its end: the second is given the object (`ctxhold6`,
  `ctxhold8`).
- A `new parser` given a context object, which the program then drops: a
  parser given the object after the drop parses, where today the program
  stops with `giving a parser a context that another parser holds`.

## Building it

This, [#667]'s runtime and [#668]'s instructions are built together, as
[#667] says: once a struct can be freed, every undo must hold what it can
put back. The cell's part of [#670]'s runtime comes with them, for a
context object's hold: the cell, its five calls, `prg->weak_pool`,
`colm_weak_clear()` in step 1 of freeing, and `makeWeaks()` with `weaks`
in `struct_el_info`, which list only a context's slot until then. The rest
of [#670]'s runtime, the instructions of `weak<T>` among it, comes after.

[#663]: https://github.com/adrian-thurston/colm-suite/issues/663
[#667]: https://github.com/adrian-thurston/colm-suite/issues/667
[#668]: https://github.com/adrian-thurston/colm-suite/issues/668
[#669]: https://github.com/adrian-thurston/colm-suite/issues/669
[#670]: https://github.com/adrian-thurston/colm-suite/issues/670
[#671]: https://github.com/adrian-thurston/colm-suite/issues/671
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#718]: https://github.com/adrian-thurston/colm-suite/issues/718
[#757]: https://github.com/adrian-thurston/colm-suite/issues/757
[#762]: https://github.com/adrian-thurston/colm-suite/issues/762
