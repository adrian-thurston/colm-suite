# Weak references: the design

Part of [#663], for [#670]. This is how a program declares a weak reference,
and how the runtime keeps one, to meet the
[requirements](struct-lifetime-requirements.md): the syntax and the type, the
cell a weak reference is, the calls on it, where each place keeps one, and
what freeing its target, backing up and exit do to it. It builds on [#667]'s
runtime model, which gives every struct a `weak_head` and frees a struct as
soon as its last strong reference goes. The instructions that read and write
a weak reference are for [#668], and how the reverse code keeps and frees
what it holds is for [#669]. A number in brackets, as (11), is a
requirement's.

## Before

- A program has no weak references. `weak` is no keyword in
  `src/colm/colm.lm`, and nothing in the repository names anything `weak`.
- A struct reference is a pointer to a `struct colm_struct`, one word
  wherever it is kept: a struct's field, a local, a parameter, a return
  value, a global, a list's or a map's value, and a tree's attribute, which
  a pointer tree (`LEL_ID_PTR`) holds.
- Nothing frees a struct before exit, so no pointer dangles.

## Declaring one

A weak reference is declared `weak<T>`, as a `ref<T>`, a `list<T>` and a
`parser<T>` are: `weak` is a keyword, and `weak<T>` a `type_ref`. `T` is
any type a strong reference can have: a struct, a context, a `list`, a
`map`, a `parser`, an `_input` or a `stream` (9). The maintainer chose this,
in planning #670, over a prefix, `weak T`.

```colm
struct node
	Name: str
	Parent: weak<node>
	Kids: list<node>
end
```

- It can be declared wherever a strong reference can (8): a struct's or a
  context's field, a local, a parameter, a function's return type, a
  global, a list's value, as in `list<weak<node>>`, a map's value, as in
  `map<str, weak<node>>`, and a def's or a token's attribute. An `alias`
  can name it.
- Colm refuses it as a map's key (8), with `a map key can't be weak`; as a
  host function's parameter or result, which the host gets and gives as C
  values, with `a host function's parameter can't be weak` and `a host
  function can't return a weak reference`; and around a type that isn't a
  struct's, as in `weak<int>`, `weak<str>`, a tree's type or
  `weak<weak<node>>`, with `weak takes a struct type`. The checks already
  there refuse it where nothing is kept: a `new`, a `cast`, a `typeid`, a
  `parse`, a `cons`, a `parser<T>`, a `ref<T>`, a `print`, an `export`, and
  a loop over a tree.
- Making `weak` a keyword is the other exception to requirement 15: a
  program that names something `weak` no longer compiles. The maintainer
  accepted it in planning #670. `ref` and `ptr` are keywords the same way.

## Its type

In the compiler, `weak<T>` is a type of its own, `TYPE_WEAK` (`type.h`),
whose `UniqueType` has `T`'s as its `target`. It is a kind beside [#710]'s:
`UniqueType::weak()`, which isn't `ptr()`, so that [#668] splits each place
three ways, as #668 says: plain values, strong struct references and weak
ones. `list<weak<node>>` and `list<node>` are different lists, since their
elements keep their values differently.

- **It reads as its target (12).** A weak slot's type is `weak<T>`, but
  reading it gives an expression a `T`, or nil: `UniqueType::readType()`.
  No expression has the type `weak<T>`, and a weak reference goes wherever
  a `T` can: a qualification through it, as `W->Name`, a call on it, as
  `W->push_tail( X )`, a test, an argument, or a value stored elsewhere. A
  call's result, a function's or a list's or a map's method's, reads the
  same way.
- **It takes a `T`, or nil.** `castAssignment()` lets a `weak<T>` slot take
  what a `T` slot takes. Storing what a weak reference reads in a strong
  place makes a strong reference, and storing a strong reference in a weak
  place makes a weak one (12).
- **A loop over weak values has a `T` variable.** `for N: node in L`, with
  `L` a `list<weak<node>>`, reads each value: its target, or nil. No loop
  over a list or a map takes a `weak<T>` variable, over weak values, strong
  values or a map's keys, since the loop gives each one as a strong
  reference. Colm refuses one with `loop variable N can't be weak: the loop
  gives each value as a strong reference`.
- **What a context or a global holds counts the same.** [#706]'s and
  [#714]'s checks read a weak type as its target: a weak reference to a
  list, a map or a struct, and a tree whose attribute is one, are held as a
  strong reference and its tree are.
- [#671]'s check takes no edge from a weak field, list or map value, or
  attribute (14).

## The cell

```c
struct colm_weak
{
	struct colm_struct *target;
	struct colm_weak *prev, *next;
};
```

A weak reference is a pointer to a cell, a `struct colm_weak`, which
[#667]'s `struct.h` declares and this design defines. The cell points at its
target, and is on the target's list of the cells that point at it, from its
`weak_head`. The list is doubly linked, so that taking a cell off it doesn't
walk the list, however many weak references the target has (#663).

- Each place that keeps a weak reference keeps one word, the cell or nil,
  where a strong reference keeps its pointer, so no slot changes size.
- A cell has one holder. No two places share one, except trees that share
  the pointer tree that holds it (below).
- Cells are allocated from a pool of their own, `prg->weak_pool`, beside
  the kids' and the trees'.
- A cell whose target is freed points at nil, and is on no list. It stays
  its holder's until the holder lets it go.

## The calls

They are in `struct.c`, declared in `struct.h` and exported, beside
[#667]'s `colm_struct_upref()` and `colm_struct_downref()`.

```c
struct colm_weak *colm_weak_new( struct colm_program *prg,
		struct colm_struct *target );
struct colm_struct *colm_weak_get( struct colm_weak *weak );
void colm_weak_set( struct colm_program *prg, struct colm_weak **slot,
		struct colm_struct *target );
void colm_weak_free( struct colm_program *prg, struct colm_weak *weak );
void colm_weak_clear( struct colm_program *prg,
		struct colm_struct *target );
```

- `colm_weak_new()` makes a cell, at the head of its target's list. Given
  nil, it makes none, and gives nil.
- `colm_weak_get()` gives the cell's target, or nil if the cell is nil or
  its target is freed (11). It counts nothing: what the VM pushes is a
  strong reference, which the instruction counts ([#667], [#668]).
- `colm_weak_set()` points a slot at a new target, for a write that is
  never undone. A cell the slot has moves from its old target's list to the
  new one's, with nothing allocated. Nil frees the slot's cell and sets the
  slot to nil. A slot that is nil gets a new cell.
- `colm_weak_free()` takes the cell off its target's list, if it has a
  target, and frees it. Given nil, it does nothing.
- `colm_weak_clear()` sets every cell on the target's list to nil, and
  empties the list. It is [#667]'s step 1 of freeing a struct.

## Where each place keeps one

Each place keeps its cell where it keeps a strong reference's pointer, and
frees it with `colm_weak_free()` where a strong reference is released
([#667], (3)).

- **A struct's field**, a context's variable, and a global, which is a
  field of `prg->global`. `struct_el_info` (`pdarun.h`) gains `weaks` and
  `weaks_len`, the offsets of the weak fields, beside [#667]'s `trees` and
  `structs`. `Compiler::makeWeaks()` (`pdabuild.cc`, new) fills them, as
  `makeTrees()` fills `trees`, and `colm_struct_delete()` frees each one's
  cell when it tears the struct down.
- **A list's or a map's value**: the element's field 0, which the element's
  `struct_el_info` lists in `weaks`, so freeing the element frees the cell.
  The list's or the map's `generic_info` has `TYPE_WEAK` as its
  `value_type`. A push, an `insert` and a `set` make a cell for the value.
  A pop, a `remove`, a `find`, a `head`, `tail` or `top`, and a loop read
  the value with `colm_weak_get()`.
- **A local or a parameter**: a word in the frame. `release_locals()`
  ([#712]) gains a kind for a weak local, beside `LI_Tree` and [#667]'s
  struct kind, which frees its cell however the frame is left (3). A weak
  parameter's cell is made when the argument is passed, and freed with the
  call's arguments.
- **A return value**: a function that returns `weak<T>` saves a cell for
  its result. Its caller reads the cell, and frees it, after the function's
  frame is torn down. So a target that only the function's locals held is
  freed by then, and the caller gets nil (10).
- **A tree's attribute**: a pointer tree, as [#667]'s struct attribute is,
  made by `colm_construct_weak_pointer()` (`tree.c`, new, declared in
  `tree.h`), with `AF_WEAK_PTR` (`0x2000`, new, `bytecode.h`) in its
  `flags` and the cell as its value. `tree_free_rec()`'s `LEL_ID_PTR` case
  frees the cell when the flag is set. A copy of the tree shares the pointer
  tree, as it does today, so the copy has a weak reference to the same
  target (13), and both read nil once the target is freed.
  `colm_cmp_tree()` doesn't compare attributes, so an attribute that turns
  nil moves no map key.

Which instructions do each of these is for [#668].

## Freeing the target

When [#667]'s `colm_struct_downref()` takes a struct's `refs` to 0, its
first step is `colm_weak_clear()`, before the struct releases what it holds.
Each weak reference to the struct reads nil from then on (11), and reading
or setting a field through one stops the program, as through any nil
struct, with `runtime_error()`'s `reading a field of a nil struct` or
`setting a field of a nil struct`. The cells stay their holders'.

A weak reference keeps nothing alive (10): no cell counts its target, so a
struct that only weak references point at is freed when its last strong
reference goes (1).

## Backing up

A weak write that the parser can back up over (`_WV`) never moves a cell:

- It makes a new cell for the new target and puts it in the slot. The
  reverse code keeps the old cell, still on its old target's list.
- Backing up frees the slot's cell and puts the old one back. The old cell
  reads its old target, or nil if that target was freed since (7).
- When the parser is freed, the reverse code frees the old cell, unless
  backing up has put it back ([#669]).

The old cell counts nothing, so the reverse code keeps no old target alive
(7), unlike what a strong write's undo keeps, which [#669] counts. A weak
attribute's set is undone as any attribute's is: the reverse code keeps the
old pointer tree, and its cell with it. An element that the reverse code
keeps keeps its cell. The `_BKT` instructions, and when the reverse code
frees what it keeps, are for [#669].

## Exit

In [#667]'s step 5, `colm_clear_heap()` tears down every struct left, in
the heap list's order, with `prg->clearing_heap` set. A target may be freed
before a cell that points at it, so while the flag is set,
`colm_weak_free()` frees a cell without taking it off a list, and
`colm_struct_downref()` calls no `colm_weak_clear()`. Nothing clears the
flag before the program is deleted, so a tree freed in step 6 frees its
cells the same way.

With `COLM_LEAK_CHECK` set, and always in a DEBUG build, the cells still
allocated after that are reported with the other pool items, as `warning:
lost weak: N`. A colm.d case that loses one fails, unless its `LOST` section
lists it, as one that loses a tree does.

## In the compiler now

The PR with this design adds `weak<T>` to the grammar and the type system,
and compiles a weak slot with a strong reference's instructions, which store
and read a pointer. No struct is freed before [#667] is built, so no weak
reference has a freed target, and each behaves as the requirements say.
[#671]'s check can be built on it.

- `src/colm/colm.lm`: the `WEAK` token, and `weak<T>` in `type_ref`.
- `loadfinal.cc`: `walkTypeRef()` makes a `TypeRef::Weak`.
- `parsetree.h`, `parsetree.cc`, `compiler.h`, `resolve.cc`,
  `synthesis.cc`, `lookup.cc`: `TypeRef::resolveTypeWeak()`, `UniqueType`'s
  `target`, `weak()` and `readType()`, a `Compiler::findUniqueType()` that
  takes a target, and the rules above.
- `type.h`: `TYPE_WEAK` (`0x0c`), which a list's or a map's `generic_info`
  has as its `value_type`.

The cell and its calls are built with [#667], [#668] and [#669], for a
context object's hold ([#669]). The rest of the runtime part, the
instructions that use them among it, comes after.

## Public interface

- Now: `type.h`, an installed header, gains `TYPE_WEAK`. Nothing compiled
  before changes.
- With the runtime part: `struct.h` defines `struct colm_weak` and declares
  the five calls, all exported; `program.h`'s `struct colm_program` gains
  `weak_pool`; `pdarun.h`'s `struct_el_info` gains `weaks` and `weaks_len`;
  `tree.h` declares `colm_construct_weak_pointer()`; and `bytecode.h`
  defines `AF_WEAK_PTR`. A program compiled by an older colm needs
  recompiling.

## Rejected

- **The list node in the slot**, three words wherever a weak reference is
  kept, with nothing allocated. Every slot is one word, and the VM's
  values, a return value and the reverse code are copied and moved, which a
  list node can't be. A cell is neither.
- **A qualifier on the slot**, a flag on the field, whose type stays `T`. A
  list's or a map's value and a return type have no field, and
  `list<weak<node>>` has to be a list of its own.
- **A prefix, `weak T`**: the maintainer chose `weak<T>`, which reads as
  `ref<T>` and `list<T>` do.
- **`weak` a keyword only where a type goes**, so that it stays free as a
  name: `weak <` in an expression could be read either way.
- **A cell shared by each copy of a value, counted, and copied when it is
  written.** Only trees share a value, and they already share the pointer
  tree. Every other copy reads the target and makes a cell of its own.
- **The undo keeping its old target**: a weak write would then keep its old
  target alive until the parser is freed, against (10).
- **A singly linked list**: taking a cell off would walk the target's list,
  as long as the target's weak references are many.

## Validation

When the runtime part is built, colm.d cases show, beside the requirements'
Validation, with `COLM_LEAK_CHECK` and `--valgrind` clean:

- A weak reference in each place, a field, a local, a parameter, a return
  value, a global, a list's and a map's value and a tree's attribute, reads
  its target while it lives, and nil after it is freed.
- A function that returns `weak<T>` of a struct that only its locals held
  gives nil.
- A slot set to a target, to nil and to another target frees its old cells:
  no `lost weak`.
- Backing up a weak write gives back the old target, and nil when it was
  freed in between, and the old cell is freed when the parser is, even
  past a commit point.
- A struct that a million weak references point at is freed, and each of
  them reads nil.

The PR with this design adds colm.d cases for the compiler part: a weak
reference in each place reads its target and takes a `T` or nil, and each
refusal above is a `COMP_ERR` case.

[#663]: https://github.com/adrian-thurston/colm-suite/issues/663
[#667]: https://github.com/adrian-thurston/colm-suite/issues/667
[#668]: https://github.com/adrian-thurston/colm-suite/issues/668
[#669]: https://github.com/adrian-thurston/colm-suite/issues/669
[#670]: https://github.com/adrian-thurston/colm-suite/issues/670
[#671]: https://github.com/adrian-thurston/colm-suite/issues/671
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#710]: https://github.com/adrian-thurston/colm-suite/issues/710
[#712]: https://github.com/adrian-thurston/colm-suite/issues/712
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
