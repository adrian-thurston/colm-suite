# Struct lifetime: the runtime model

Part of [#663], for [#667]. This is how the runtime counts the references
to a struct and frees it, to meet the
[requirements](struct-lifetime-requirements.md): the struct's header, the
calls that count and free, what freeing each kind of struct releases, the
references the runtime's own C code holds, streams, and exit. The
instructions that take and drop references are for [#668], backing up is
for [#669], and what a weak reference is, is for [#670]. A number in
brackets, as (4), is a requirement's.

## Before

- `colm_struct_new()` and `colm_struct_add()` (`struct.c`) put each struct
  on the program's heap list, `prg->heap`. Only `colm_clear_heap()`
  (`program.c`), at exit, frees what is on it, apart from the elements of
  lists and maps (below).
- `colm_struct_delete()` releases a struct's trees: the fields its
  `struct_el_info` (`pdarun.h`) lists in `trees`, which
  `Compiler::makeTrees()` (`pdabuild.cc`) fills with the fields whose type
  is a tree. A field that holds a struct is in no list. A list, a map, a
  parser, an `_input` and a stream run their destructor instead. A list's,
  `colm_list_destroy()` (`list.c`), is empty, and a map has none.
- A list's or a map's element is a struct of its own, on the heap list.
  Since [#699], no program names one. A pop, a `remove`, and an `insert` of
  a key the map has, free it at once (`el_free()`, `bytecode.c`), unless
  the parser's reverse code keeps it.

## The header

```c
struct colm_weak;

struct colm_struct
{
	short id;
	long refs;
	struct colm_weak *weak_head;
	struct colm_struct *prev, *next;
};
```

- `refs` counts the strong references to the struct, as a tree's `refs`
  does.
- `weak_head` is the first of the struct's weak references, which are
  threaded onto a list of their own, as #663 decided. `struct colm_weak`
  is declared here and defined by [#670]. Until then, `weak_head` is
  always nil.
- `prev` and `next` link the struct on the heap list while it lives, and
  on the list of structs to free while it is being freed (below).

`struct colm_inbuilt`, `parser_t`, `input_t`, `stream_t`, `list_t` and
`map_t` (`struct.h`) each repeat the header, in this order, as they do
today. Every struct and every element is two words larger.

## Elements

A list's or a map's element belongs to its list or map alone: no program
names one, so nothing else holds it. It isn't counted, and isn't on the
heap list.

- `colm_struct_new_el()` (`struct.c`, new) makes an element as
  `colm_struct_new()` makes a struct, but leaves it off the heap list. Its
  `refs` stays 0. Every element is made with it: in `colm_vlist_append()`
  and `colm_vlist_prepend()` (`list.c`), `colm_vmap_insert()` (`map.c`),
  `FN_VMAP_INSERT_WC` and `_WV`, `FN_VMAP_SET_WC` and `_WV`, and
  `construct_argv()` and `construct_stds()` (`bytecode.c`), which call
  `colm_struct_new_size( prg, 16 )` today.
- An element is freed when its list or map lets it go: by a pop, a
  `remove`, an `insert` of a key the map has, or the undo of what added
  it, as today, or by the list or map being freed. `el_free()` no longer
  calls `heap_unlink()`.
- In a parse's actions, a pop or a `remove` gives its element to the
  reverse code, which frees it when the parser goes, and puts it back when
  the parser backs up, as today. The `heap_unlink()` and
  `colm_struct_add()` calls in `FN_VLIST_POP_HEAD_WV` and `_BKT`,
  `FN_VLIST_POP_TAIL_WV` and `_BKT`, and `FN_VMAP_REMOVE_WV` and `_BKT`
  go: the element is never on the heap list.
- An element's value, and a map element's key, are its fields, released
  as any struct's fields are (below).

## Counting

```c
void colm_struct_upref( struct colm_program *prg,
		struct colm_struct *s );
void colm_struct_downref( struct colm_program *prg,
		struct colm_tree **sp, struct colm_struct *s );
```

Both are in `struct.c`, declared in `struct.h` and exported, as
`colm_tree_upref()` and `colm_tree_downref()` are.

- Each takes nil and does nothing with it.
- `colm_struct_upref()` adds one to `refs`. `colm_struct_downref()` takes
  one from it, and frees the struct when it reaches 0 (1).
- Each asserts that the id is a struct's (`first_struct_el_id` or above),
  as the tree calls assert that it is a tree's, and the downref that
  `refs` is above 0.
- A struct starts with `refs` at 0, as a tree does. What first holds it
  counts that reference: the program its global, the VM a `new` ([#668]).
- An element is never counted: neither call is given one.

## Freeing

### The order

When `colm_struct_downref()` takes a struct's `refs` to 0:

1. The struct's weak references are set to nil, by a call that [#670]
   defines, so that no weak reference reads it from here on (11).
2. The struct is taken off the heap list, by `heap_unlink()`, which moves
   from `bytecode.c` to `struct.c`.
3. The struct goes on the program's list of structs to free,
   `prg->free_structs`, linked through its `next`.
4. Unless a call further out is freeing structs already
   (`prg->freeing_structs` is set), the call sets the flag, then tears down
   and frees the structs on the list, one at a time, until the list is
   empty, then clears the flag. Each teardown (below) releases what the
   struct holds. A struct that a teardown takes to 0 goes through steps 1
   to 3, and the same loop frees it.

So freeing never recurses in C from one struct to the next, however long
a chain of structs or however long a list is (4). A tree a teardown
releases is freed by `tree_free_rec()`, which uses the VM stack, as today.
A struct that the tree holds in a pointer tree (below) goes on the list.
If a tree freed outside a teardown leads to a struct, the downref frees
the list from inside `tree_free_rec()`. The trees freed then lead to
structs only through the list, so C recursion is at most one level deep.

No program code runs while structs are being freed. A stream's close
(below) can stop the program part way, through `runtime_error()`, which
ends the process.

### What each kind releases

`colm_struct_delete()` tears a struct down, then frees its memory. By
kind:

- A user struct, a context, the global, and an element: each field that
  `struct_el_info` lists, a tree with `colm_tree_downref()`, and a struct
  with `colm_struct_downref()`.
- A list, in `colm_list_destroy()`: each element, from the head, in a
  loop: its fields as above, then its memory.
- A map, in `colm_map_destroy()` (`struct.c`, new, `static` beside
  `colm_map_new()`, which sets it as the destructor, as
  `colm_list_destroy()` is beside `colm_list_new()` in `list.c`): each
  element, in key order through the elements' `next`, the same way.
- A parser, in `colm_parser_destroy()`: its context (`pda_run->context`),
  taken before its PDA run is freed, its PDA run (`colm_pda_clear()`) and
  its `result` tree, as today, and its `input`.
- An `_input`, in `colm_input_destroy()`: its buffers, as today, and the
  stream each source buffer holds (below).
- A stream, in `colm_stream_destroy()`: it closes the stream and frees its
  `impl`, as today (below).

`struct_el_info` gains `structs` and `structs_len`, the offsets of the
fields that hold a struct, beside `trees` and `trees_len`.
`Compiler::makeStructs()` (`pdabuild.cc`, new) fills them, as
`makeTrees()` fills `trees`, with the fields whose type is `ptr()`: a
struct, a context, a list, a map, a parser, an `_input` or a stream. A
map element's key is one when the key type is a struct, as in a
`map<s, int>`, just as a tree key is in `trees` today. `pdacodegen.cc`
writes the offsets out beside the trees'. A weak field ([#670]) is in
neither list.

## The references the runtime holds

Each place in the runtime's C code that keeps a struct holds a strong
reference to it (2): it counts what it stores, and releases what it lets
go of or overwrites.

- **The program's global**, `prg->global`: counted in
  `colm_alloc_global()` (`program.c`), released at exit (below).
- **`stdin`, `stdout` and `stderr`**, `prg->stdin_val`, `stdout_val` and
  `stderr_val`: counted in `make_stdin()`, `make_stdout()` and
  `make_stderr()` (`bytecode.c`), released at exit.
- **`argv` and `stds`**, lists in the global's fields: counted in
  `FN_LOAD_ARGV` and `FN_INIT_STDS`, released when the global is freed.
  `stds`'s element holds `stdout`, counted in `construct_stds()`.
- **A parser's `input`**: counted in `colm_construct_generic()` and
  `colm_construct_reducer()` (`struct.c`), and in `IN_SET_PARSER_INPUT`
  (`bytecode.c`), which releases the input it replaces, and released when
  the parser is freed. `IN_SET_PARSER_INPUT` runs for each parse given an
  `_input`, right after the parser is made with an input of its own: a
  token action's `parse_stop H: hash(input)[]` gives the new parser the
  input of the parser the action belongs to, which both then hold.
- **A parser's context**, `pda_run->context`: counted in
  `colm_parser_set_context()` (`bytecode.c`), which releases the context
  it replaces, and released when the parser is freed.
- **An `_input`'s source buffer**, on the stream it reads: `struct
  seq_buf` (`input.h`) gains `stream`, beside the `si` it keeps today.
  Counted in `input_prepend_stream()` and `input_append_stream()`
  (`input.c`), released in `free_seq_buf()`, when the push or the send is
  undone, or the input is freed.
- **A `for` loop over a list, a map or `map_keys()`**, on what it is over:
  the iterator's `container` ([#715]). Counted in `IN_GEN_ITER_FROM_REF`,
  beside the loop count, and released in `IN_GEN_ITER_UNWIND` and
  `IN_GEN_ITER_DESTROY`, once, under the same test of the iterator's type
  as the loop count. While a loop is over a list or a map, no change to it
  goes ahead, so the loop holds its elements by holding it. A loop that a
  user iterator's teardown stops short of keeps its reference, as it keeps
  its count ([#734]): its list or map is then freed only at exit, and is
  counted lost.
- **A pointer tree**, on the struct a tree's attribute holds (below).

A parser is held while it parses by the send that runs it: `IN_SEND_*_W`
and `IN_PARSE_FRAG_W` keep it on the VM stack, as they do today. The VM
stack, locals, arguments, return values, struct fields, globals and a
context's variables are for [#668], and what the reverse code keeps is for
[#669].

## Streams

Freeing a stream closes it, as `close()` does: the maintainer's choice, in
planning #667, over keeping a dropped stream's file open until exit.
Requirement 15 says so.

- `colm_stream_destroy()` is as today. `data_destructor()` closes the
  file, which writes out what the stream holds, then frees the `impl`. A
  close that fails stops the program through `runtime_error()`, at what
  dropped the stream: an assignment, a local going out of scope, or its
  list, map, struct or input being freed.
- So a stream that the program drops without closing it is written out
  then, not at exit (15). A stream held by a local of the top-level code
  is dropped when that code ends, as `colm_execute()` releases its locals.
  What a global holds is closed at exit, in the order the streams were
  opened, as today (below).
- An input holds each stream pushed or sent into it, so a stream that a
  token action opens and pushes is read after the statement that opened it
  has ended.
- `stdin`, `stdout` and `stderr` are the program's until exit.
- When this is built, the manual's "when the program ends, which closes
  every stream it left open" (`3_06_send.adoc`) changes to say that a
  stream is closed when the program drops it.

## A struct in a tree's attribute

A tree holds a value attribute as a pointer tree (`LEL_ID_PTR`,
`colm_construct_pointer()`, `tree.c`), which copies of the tree share
through its `refs`. A pointer tree that holds a struct holds a strong
reference to it, so the struct lives as long as any tree that shares it
(5).

- `colm_construct_struct_pointer()` (`tree.c`, new, declared in `tree.h`)
  makes a pointer tree as `colm_construct_pointer()` does, sets
  `AF_STRUCT_PTR` (`0x1000`, new, `bytecode.h`) in its `flags`, and counts
  the struct. `colm_construct_pointer()` stays, for an `int` or a `bool`.
  Which instructions call which is for [#668].
- `tree_free_rec()`'s `LEL_ID_PTR` case releases the struct when the flag
  is set, then frees the pointer tree.
- A copy of a tree shares the pointer tree, as today. Comparing and
  printing a pointer tree are as today.

## Exit

`colm_delete_program()` (`program.c`) ends a run in this order:

1. It releases `prg->return_val`, as today.
2. It closes every stream still open, in the order they were made, which
   writes out what each holds. This is `close_streams()`, which `exit_now()`
   uses today. It leaves `bytecode.c`'s statics as `colm_close_streams()`,
   declared in `rterror.h`. A close that fails stops the program, as today.
3. It releases the program's own references: the global, then `stdin`,
   `stdout` and `stderr`, each set to nil first. This frees everything they
   lead to.
4. With `COLM_LEAK_CHECK` set, and always in a DEBUG build, it counts the
   structs still on the heap list, and reports them before the pool items,
   as `warning: lost structs: N` (16). The elements of a lost list or map
   aren't counted: they aren't structs.
5. It frees what is left, with `colm_clear_heap()`, with
   `prg->clearing_heap` set. Each struct is torn down as above, but
   `colm_struct_downref()` does nothing while the flag is set: every struct
   left is on the heap list, and the same walk frees it, so none is freed
   twice. The walk clears no weak reference ([#670]). Trees, elements,
   buffers and PDA runs are released as usual.
6. It releases `prg->error`, reports the lost pool items and frees the
   pools, as today.

A struct is lost only when a reference is counted and never released, or
when a cycle of strong references holds it, which [#671]'s check rules out
(14). `exit_now()` closes the streams and ends the process, and
`runtime_error()` ends it, freeing and counting nothing, as today.

## For the other designs

- [#668]: each instruction that stores a struct reference counts it, and
  each one that drops one releases it: values on the VM stack, locals and
  arguments, return values, struct fields, globals, a context's
  variables, list and map values and keys, and a tree's attribute, through
  `colm_construct_struct_pointer()`. A frame's struct locals are released
  in `release_locals()` ([#712]), with a local kind for a struct beside
  `LI_Tree`. A `new` gives the VM the struct's first reference.
- [#669]: what the reverse code keeps of a struct, such as a field's old
  value or a parser it sends to, is a strong reference. An element that
  the reverse code keeps isn't counted: it is the reverse code's alone, as
  today. `stream_undo_append_stream()` doesn't use the stream that
  `IN_SEND_STREAM_BKT` records: the input's buffer holds the stream, and
  the undo frees the buffer.
- [#670]: `struct colm_weak`, the call that sets a struct's weak
  references to nil (step 1 of freeing), and where a weak field goes in
  `struct_el_info`. A weak reference held in a struct, an element or a
  tree's attribute is taken off its target's list when its holder is
  freed, but not in exit's step 5.
- [#671] has no runtime part.

## Building it

- This and the designs of [#668] and [#669] are built together. Once a
  struct can be freed, every holder must count its reference, and every
  undo must hold what it can put back. Weak references ([#670]) come
  after, as #663's plan has them.
- The lost-structs report fails each test case that loses a struct. So
  [#671]'s check comes before the report, or with it.
  `test/colm.d/lookup1.lm` builds strong cycles: a `lang_object` holds
  its children in `objectMap`, and each child holds it as
  `lookupParent`.
- `test/colm.d/elfree1.lm`'s host counts the structs on the heap list, and
  elements are no longer on it, so that case needs another way to show
  that an element is freed.

## Public interface

Installed headers change, and a program compiled by an older colm needs
recompiling, as after [#699] and [#715]:

- `struct.h`: the header gains `refs` and `weak_head`, in
  `struct colm_struct` and the six structs that repeat it. `struct
  colm_weak` is declared. `colm_struct_upref()`, `colm_struct_downref()`
  and `colm_struct_new_el()` are new.
- `pdarun.h`: `struct_el_info` gains `structs` and `structs_len`.
- `input.h`: `struct seq_buf` gains `stream`.
- `program.h`: `struct colm_program` gains `free_structs`,
  `freeing_structs` and `clearing_heap`.
- `tree.h`: `colm_construct_struct_pointer()` is new.
- `bytecode.h`: `AF_STRUCT_PTR` is new.

libcolm exports each function that isn't `static`, so its exported
symbols change too: the new functions above, and `colm_close_streams()`,
which `close_streams()` becomes. That one is declared in `rterror.h`,
the runtime's own header, which isn't installed, beside
`colm_stream_error()`, which is exported the same way.
`colm_map_destroy()` is `static` in `struct.c`, so it adds nothing.
`colm_struct_delete()` keeps its signature.

## Rejected

- Keeping a dropped stream's file open until exit, with each stream's
  `impl` on a list of the program's. It kept requirement 15 as it was, but
  a dropped stream's file stayed open to exit. The maintainer chose to
  close it, and to change requirement 15.
- The program holding every stream until exit: against (1).
- Freeing recursively in C, struct to struct: a chain of a million
  structs, which a program can build until [#671]'s check lands, would
  exhaust the C stack (4). The list of structs to free needs no memory of
  its own, and doesn't depend on [#671].
- Counting elements: only an element's list or map holds it, so its count
  would always be 1.
- Leaving elements on the heap list: exit's step 5 would free a lost
  list's elements twice, through the list and through the walk, and they
  would be counted as lost structs.
- A smaller header for elements, without `refs` and `weak_head`: it would
  save two words an element, but every access to an element's fields goes
  through `colm_struct_get_field()`, which takes the one header's size. It
  can come later.
- A new tree id for a pointer tree that holds a struct: the reserved ids,
  `LEL_ID_PTR` among them, come before the grammar's, so a new one moves
  every id. A flag needs no id.

## Validation

When this is built, colm.d cases show, besides the requirements'
Validation, with `COLM_LEAK_CHECK` and `--valgrind` clean:

- A `for` loop over a global list, whose called function gives the global
  a new list, goes on over the old list, which is freed when the loop ends.
- A stream that a token action opens and pushes is read after the
  statement that opened it has ended, and is freed with the parse's input.
- A stream dropped without a close is written out at the drop: a file
  reads back what was sent to it, and a stream on `/dev/full` stops the
  program there, before what the program prints after it.
- A list of a million structs, and a chain of a million, are freed without
  exhausting the C stack.
- A struct in a tree's attribute lives as long as a copy of the tree does.
- A map's struct key is released with its element, and a parser's context
  and input with the parser.
- A parse given an input, as a token action's `parse_stop H: hash(input)[]`
  is, frees the input it was made with, and leaves the input it was given
  with the parser that holds it.
- A host that counts a reference to the program's global before
  `colm_delete_program()` gets `warning: lost structs: N`, N the structs
  the global leads to.

[#663]: https://github.com/adrian-thurston/colm-suite/issues/663
[#667]: https://github.com/adrian-thurston/colm-suite/issues/667
[#668]: https://github.com/adrian-thurston/colm-suite/issues/668
[#669]: https://github.com/adrian-thurston/colm-suite/issues/669
[#670]: https://github.com/adrian-thurston/colm-suite/issues/670
[#671]: https://github.com/adrian-thurston/colm-suite/issues/671
[#699]: https://github.com/adrian-thurston/colm-suite/issues/699
[#712]: https://github.com/adrian-thurston/colm-suite/issues/712
[#715]: https://github.com/adrian-thurston/colm-suite/issues/715
[#734]: https://github.com/adrian-thurston/colm-suite/issues/734
