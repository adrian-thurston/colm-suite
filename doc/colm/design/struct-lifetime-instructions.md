# Struct lifetime: the instructions

Part of [#663], for [#668]. This is which instructions count and release a
struct reference, and which read and write a weak one, to meet the
[requirements](struct-lifetime-requirements.md). It uses the calls of
[#667]'s [runtime model](struct-lifetime-runtime.md) and of [#670]'s
[weak references](weak-reference-design.md), and starts from the
[instructions for each place a value lives](access-instructions.md). What
the reverse code keeps, and what each `_BKT` instruction does, is for
[#669]. A number in brackets, as (2), is a requirement's.

## Before

- No instruction counts a struct. The compiler gives a struct, a context,
  a list, a map, a parser, an `_input` and a stream (`UniqueType::ptr()`)
  the instructions it gives an `int` and a `bool` (`scalar()`):
  `IN_GET_LOCAL_VAL_R` and `IN_SET_LOCAL_VAL_WC`, `IN_GET_STRUCT_VAL_R`
  and `IN_SET_STRUCT_VAL_WC`, `_WV` and `_BKT`, `IN_GET_FIELD_VAL_R`,
  `IN_SET_FIELD_VAL_WC` and `_WV`, `IN_VAL_TO_POINTER`, `IN_POP_VAL`,
  `IN_DUP_VAL`, the six `IN_TST_*_VAL` comparisons, `IN_NOT_VAL`,
  `IN_JMP_FALSE_VAL` and `IN_JMP_TRUE_VAL`. `LangVarRef::loadField()` and
  `setField()` (`synthesis.cc`) take them with `ptr() || scalar()`, and
  [#670]'s compiler part adds `weak()` to the same tests.
- An instruction that pops a struct as its object, the struct whose field
  it reads or sets, or the list it pushes to, drops it.
- `Compiler::findLocals()` lists only a frame's tree locals, as `LT_Tree`.
  So `release_locals()` (`bytecode.c`) releases nothing that a struct
  local or parameter holds, however its frame is left.
- A list's and a map's instructions serve every value type. They read the
  generic's `value_type` and `key_type` (`struct generic_info`, `tree.h`),
  and count only a `TYPE_TREE` value.

## The rule

A struct on the VM stack is a strong reference (2), as a tree on it is:

1. An instruction that pushes a struct counts it, with
   `colm_struct_upref()` ([#667]): a read of a place, a root (the global,
   a context, `input`, `stdin`, `stdout` and `stderr`), and a list's or a
   map's value or key. One that makes a struct gives the VM its first
   reference: `IN_NEW_STRUCT`, `IN_NEW_STREAM`, `IN_CONS_GENERIC`,
   `IN_CONS_REDUCER` and `IN_OPEN_FILE`. One that takes a value out of a
   place that lets it go moves the place's reference to the stack: a pop,
   a `remove`, and a return value.
2. An instruction that stores a struct moves the stack's reference into
   the place, then releases what the place held, with
   `colm_struct_downref()`. Where the runtime's C code counts what it is
   given (`colm_parser_set_context()`, `input_append_stream()`,
   `input_prepend_stream()` and `colm_construct_struct_pointer()`, by
   [#667]), the instruction releases the stack's reference after it.
3. An instruction that pops a struct and keeps nothing of it releases it
   once it is done with it: its object, a map's key it looks up, an
   operand it tests or compares, and a value a statement drops.
4. An instruction that pops a struct and pushes it back, as a send pushes
   its parser back, moves the reference and counts nothing.
5. Each counts what it pushes or keeps before it releases anything. A
   release can free a struct and what it leads to (1, 4), so nothing is
   read from an object or an old value after it is released.
6. An `int` and a `bool` keep their instructions, which do with them what
   they do today.

No program code runs while structs are freed ([#667]), but a stream's
close can stop the program, through `runtime_error()`, at the instruction
that dropped the stream (15).

## Weak slots

A weak slot keeps a cell ([#670]), but for a weak attribute that
`make_token` gave a struct, which holds the struct as a strong one does
(below). Nothing on the VM stack is weak: no expression has a weak type
(12).

- A read gives the cell's target, counted, or nil: `colm_weak_get()`,
  then `colm_struct_upref()`.
- A write takes a strong reference from the stack. One that is never
  undone (`_WC`) points the slot's cell at it, with `colm_weak_set()`. One
  that can be (`_WV`) puts a new cell in the slot, from `colm_weak_new()`,
  and gives the old cell to the reverse code. Then the write releases the
  stack's reference, so a struct that only the stack held is freed there,
  and the slot reads nil (10).
- A slot that is let go of frees its cell, with `colm_weak_free()`.

## In the compiler

Each place a struct can be splits three ways by its slot's type: a
`scalar()` takes its instruction as today, a `ptr()` its strong
instruction, and a `weak()` its weak one.

- `ObjectField` (`parsetree.h`) gains `inGetPtrR`, `inSetPtrWC` and
  `inSetPtrWV`, and `inGetWeakR`, `inSetWeakWC` and `inSetWeakWV`, each
  `IN_HALT` until it is set. `ObjectField::initField()` sets them for a
  local, a struct's field and a tree's attribute (below). A struct is only
  read on the way to a write: a struct in a qualification is at or before
  its last pointer, which is loaded with its read. So neither kind has a
  slot for a load for writing.
- `LangVarRef::loadField()` takes `inGetPtrR` for a `ptr()` field, whether
  it loads for writing or not, and `inGetWeakR` for a `weak()` one.
  `LangVarRef::setField()` takes `inSetPtrWV` or `inSetPtrWC`, and
  `inSetWeakWV` or `inSetWeakWC`, by the field's type, as it takes the
  value slots today. The weak instructions are `FN_` codes (below), so
  both write `IN_FN` before one.
- The built-in fields that hold a struct set `inGetPtrR` to what they set
  `inGetValR` to today: `addInput()`'s `IN_LOAD_INPUT_R`, `addThis()`'s
  `IN_LOAD_CONTEXT_R`, and the `IN_GET_CONST` of `addStdin()`,
  `addStdout()` and `addStderr()`. `initListFieldVal()`'s `head`, `tail`
  and `top` set it to `IN_GET_VLIST_MEM_R`. They are declared with the
  value's read type (`readType()`), so a `list<weak<T>>`'s `head` is a `T`
  field, which `IN_GET_VLIST_MEM_R` reads by the list's `value_type`.
- The other places the compiler picks an instruction by kind are under
  each place below.

## The instructions

The one-byte opcode space has 23 free values, with [#744]'s `IN_EXPR_WORDS`,
which has the value [#743]'s `IN_POP_EXPR` had.
The strong instructions take 14 of them. The weak ones run far less often,
and are two-byte codes under `IN_FN`, named `FN_`, as a list's and a map's
are. Their values are the implementation's: the next free ones in
`bytecode.h`.

### Strong

Each takes the operands of the `_VAL_` or `_TREE` instruction it stands
beside. `IN_PTRS_TO_VALS` takes none.

- `IN_GET_LOCAL_PTR_R`: pushes a local's or a parameter's struct, counted.
- `IN_SET_LOCAL_PTR_WC`: pops a struct into a local, and releases the
  struct the local held.
- `IN_GET_STRUCT_PTR_R`: pops a struct, pushes the struct its field holds,
  counted, then releases the first.
- `IN_SET_STRUCT_PTR_WC`: pops a struct and a value, stores the value in
  the struct's field, releases the field's old value, then the struct.
- `IN_SET_STRUCT_PTR_WV`: the same, but the old value goes to the reverse
  code, with `IN_SET_STRUCT_PTR_BKT` and the field, and the record takes
  over its reference ([#669]).
- `IN_SET_STRUCT_PTR_BKT`: the undo, for [#669].
- `IN_GET_FIELD_PTR_R`: pops a tree, pushes the struct that its
  attribute's pointer tree holds, counted, then releases the tree.
- `IN_SET_FIELD_PTR_WC`: pops a tree and a struct, makes a pointer tree
  for the struct with `colm_construct_struct_pointer()` ([#667]), releases
  the stack's struct, sets the attribute, releases its old pointer tree,
  then the tree.
- `IN_SET_FIELD_PTR_WV`: the same, but the old pointer tree goes to the
  reverse code with `IN_SET_FIELD_TREE_BKT`, as `IN_SET_FIELD_VAL_WV`'s
  does.
- `IN_PTR_TO_POINTER`: pops a struct, pushes a pointer tree for it, as
  `IN_VAL_TO_POINTER` does for an `int`, then releases the struct.
- `IN_POP_PTR`: pops a struct and releases it.
- `IN_DUP_PTR`: counts the struct on top, and pushes it again.
- `IN_TST_NZ_PTR`: pops a struct, pushes whether it isn't nil, as a
  `bool`, then releases it, as `IN_TST_NZ_TREE` does a tree.
- `IN_PTRS_TO_VALS`: pops the two structs on top, pushes their addresses
  in the same order, as plain words, then releases both.

### Weak

- `FN_GET_LOCAL_WEAK_R` and `FN_GET_STRUCT_WEAK_R`: read a weak local or
  field as the strong ones do, and push its target, counted, or nil.
- `FN_GET_FIELD_WEAK_R`: reads a weak attribute as `IN_GET_FIELD_PTR_R`
  reads a strong one, but by its pointer tree's flag. A pointer tree with
  `AF_WEAK_PTR` holds a cell ([#670]), and gives the cell's target, through
  `colm_weak_get()`, or nil. One with `AF_STRUCT_PTR` holds the struct
  itself, as `make_token` puts one in (below), and gives that struct.
  Either way it counts what it pushes.
- `FN_SET_LOCAL_WEAK_WC`, `FN_SET_STRUCT_WEAK_WC` and
  `FN_SET_FIELD_WEAK_WC`: write a weak local, field or attribute as the
  strong ones do: a local or a field with `colm_weak_set()`, and an
  attribute with a new pointer tree from `colm_construct_weak_pointer()`
  and `colm_weak_new()`.
- `FN_SET_STRUCT_WEAK_WV` and `FN_SET_FIELD_WEAK_WV`: the same, undone. A
  field gets a new cell, and the reverse code the old one, with
  `FN_SET_STRUCT_WEAK_BKT` ([#669]). An attribute's old pointer tree goes
  to the reverse code as a strong attribute's does.
- `FN_SET_STRUCT_WEAK_BKT`: the undo, for [#669].
- `FN_WEAK_TO_POINTER`: pops a struct, pushes a pointer tree with a new
  cell for it, then releases the struct.
- `FN_STASH_ARG_WEAK`: pops a struct into a weak parameter's argument
  slot, as a new cell, then releases it.
- `FN_SAVE_RET_WEAK`: pops a struct into the frame's return value, as a
  new cell, then releases it.
- `FN_LOAD_RETVAL_WEAK`: pushes the return value's target, counted, or
  nil, and frees its cell.

## Each place

### Locals and parameters

- A local, a value parameter, `lhs` and `r1` to `rN`
  (`ObjectField::UserLocalType`, `ParamValType`, `LhsElType` and
  `RedRhsType`) take `IN_GET_LOCAL_PTR_R` and `IN_SET_LOCAL_PTR_WC`, or
  `FN_GET_LOCAL_WEAK_R` and `FN_SET_LOCAL_WEAK_WC`. A write to a local is
  never undone, so neither has a `_WV`.
- `findLocals()` lists a `ptr()` local or parameter as `LT_Struct`, and a
  `weak()` one as `LT_Weak`, beside `LT_Tree`. `LocalType` (`parsetree.h`)
  and the `LI_` kinds (`pdarun.h`) gain them as 2 and 3.
  `release_locals()` releases an `LI_Struct` with `colm_struct_downref()`,
  and frees an `LI_Weak`'s cell with `colm_weak_free()`. So a struct local
  or parameter is released however its frame is left (3): a return, the
  end of an action, one that rejected among them, an `exit`'s walk, and a
  user iterator's teardown ([#712]).
- A local keeps its struct until it is overwritten or its frame is left,
  as a tree local does, so one declared in a block keeps it past the
  block's end. The maintainer chose this, in planning #668, over releasing
  it at the end of its block too. A local declared with no value in a loop
  sees what the last pass left in it, as it does a tree today. So the
  "goes out of scope" of (3) is its frame being left.
- A weak parameter gets its cell when the argument is passed:
  `evaluateArgs()` writes `FN_STASH_ARG_WEAK` where it writes
  `IN_STASH_ARG` today. A strong argument is stashed as today, which moves
  the stack's reference into the argument slot.

### Fields of structs, globals and a context's variables

- `ObjectField::StructFieldType`, `GenericElementType` and
  `GenericDependentType` take `IN_GET_STRUCT_PTR_R`, `IN_SET_STRUCT_PTR_WC`
  and `IN_SET_STRUCT_PTR_WV`, or the weak ones. A global is a field of
  `prg->global`, and a context's variable a field of the context.
- Freeing a struct releases what its fields hold, by [#667]'s `structs` and
  [#670]'s `weaks` in `struct_el_info`.
- The instructions for a tree or an `int` in a field, `IN_GET_STRUCT_R`,
  `_WC` and `_WV`, `IN_SET_STRUCT_WC` and `_WV`, `IN_GET_STRUCT_VAL_R`, and
  `IN_SET_STRUCT_VAL_WC` and `_WV`, do with the value what they do today,
  and release the struct they pop once they are done with it (3).

### The roots

- `IN_LOAD_GLOBAL_R`, `_WC` and `_WV`, `IN_LOAD_CONTEXT_R`, `_WC` and
  `_WV`, and `IN_LOAD_INPUT_R`, `_WC` and `_WV` count what they push: the
  global, the parser's context and the parser's input.
- `IN_GET_CONST` releases the global it pops, and counts the stream it
  pushes for `stdin`, `stdout` and `stderr`.
- The instructions that pop the global as their object release it:
  `IN_GET_ERROR`, `IN_SYSTEM`, `FN_SPRINTF` and `FN_EXIT`. `FN_EXIT_HARD`
  ends the process.
- `IN_PTR_ACCESS_WV`, the root of a write undone through a pointer, pops
  its struct and pushes it back. It records the pointer, which the record
  holds as [#669] says.

### Attributes of trees

- An attribute that holds a struct (`ObjectField::UserFieldType`) takes
  `IN_GET_FIELD_PTR_R`, `IN_SET_FIELD_PTR_WC` and `IN_SET_FIELD_PTR_WV`,
  and a weak one `FN_GET_FIELD_WEAK_R`, `FN_SET_FIELD_WEAK_WC` and
  `FN_SET_FIELD_WEAK_WV`. `IN_GET_FIELD_VAL_R` and `IN_SET_FIELD_VAL_WC`
  and `_WV` stay, for an `int` and a `bool`.
- A constructor's attribute argument is wrapped by the attribute's type,
  as `setField()` picks by the field's. `LangTerm::evaluateConstruct()`
  looks the attribute up as `assignFieldArgs()` does (`findFieldNum()`),
  and writes `IN_VAL_TO_POINTER` for an `int` or a `bool`,
  `IN_PTR_TO_POINTER` for a struct and `FN_WEAK_TO_POINTER` for a weak
  one. A tree and a nil go in as they are. Today it picks by the
  argument's type.
- A `make_token` argument goes into the slot its position gives at run
  time, whatever its type ([#477]). So `evaluateMakeToken()` wraps it by
  the argument's type, a struct with `IN_PTR_TO_POINTER`, and a weak
  attribute that `make_token` gives a struct holds it strongly.
  `FN_GET_FIELD_WEAK_R` reads such an attribute as the struct it holds
  (above). This is an exception to (10) and (12): the attribute keeps its
  target alive while the tree holds it, and never reads nil, as [#671]'s
  check already counts it, since that check takes every `make_token`
  argument as a strong reference. Colm knows the slot when it compiles the
  call only for an id written out as `typeid<T>`. For an id computed at
  run time, wrapping by the slot's kind would need a table of each
  attribute's kind, which the runtime doesn't keep. So this design keeps
  one rule for both.
- [#745] checks the tree's type in every instruction that reads or sets
  an attribute, against the id of the object of the type that declares
  the attribute. Each new attribute instruction checks it as the `_VAL_`
  instruction it stands beside does: `IN_GET_FIELD_PTR_R`,
  `IN_SET_FIELD_PTR_WC` and `_WV`, `FN_GET_FIELD_WEAK_R`, and
  `FN_SET_FIELD_WEAK_WC` and `_WV`. `IN_GET_FIELD_VAL_R` and the other
  `_VAL_` instructions keep the check. Whichever of [#745]'s fix and this
  design's implementation lands second adds it to the new instructions.

### Lists and maps

A list's and a map's instructions keep serving every value type, and
dispatch on `value_type` and `key_type` at run time, as they do for a tree
today. Three calls do it, in `struct.c`, declared in `struct.h` and
exported:

```c
value_t colm_value_get( struct colm_program *prg, enum TYPE type,
		value_t value );
value_t colm_value_put( struct colm_program *prg, struct colm_tree **sp,
		enum TYPE type, value_t value );
void colm_value_release( struct colm_program *prg, struct colm_tree **sp,
		enum TYPE type, value_t value );
```

- `colm_value_get()` gives what a read pushes of a value an element holds:
  a tree or a struct, counted; a weak value's target, counted, or nil; and
  an `int` or a `bool` as it is.
- `colm_value_put()` gives what an element keeps of a value the VM gives
  it: the same reference, moved, or for a weak value a new cell, after
  which it releases the stack's reference.
- `colm_value_release()` lets go of a value: a tree or a struct with its
  downref, and a weak value's cell with `colm_weak_free()`.

Each instruction uses them where it tests for `TYPE_TREE` today:

- `colm_vlist_detach_head()` and `_tail()` (`list.c`),
  `colm_viter_deref_cur()` (`iter.c`), `colm_vmap_find()` (`map.c`),
  `IN_GET_VLIST_MEM_R` and `IN_GEN_KITER_GET_CUR_R` read with
  `colm_value_get()`.
- A push, an `insert` and a `set` store the value, and an `insert`'s and a
  `set`'s key, with `colm_value_put()`.
- A pop and a `remove` read the value with `colm_value_get()`, then free
  the element, which releases what it holds ([#667]): the value moves to
  the stack.
- `remove`, `contains` and `find` release the key they look up with
  `colm_value_release()`.
- A `set` of a key the map has gives the found element the new value, and
  releases the old one with `colm_value_release()` under `_WC`, or gives it
  to the reverse code under `_WV`. The new element's value is cleared
  before it is freed, as today, and freeing it releases its key. An
  `insert` of a key the map has frees the new element, which releases the
  key and the value it took.
- Each releases the list or the map it pops, last, and so do
  `IN_LIST_LENGTH` and `IN_MAP_LENGTH`.

A map's key can't be weak ([#670]), so `key_type` is never `TYPE_WEAK`.

### Loops

- A loop over a list's or a map's values, or a map's keys, reads each with
  `colm_viter_deref_cur()` or `IN_GEN_KITER_GET_CUR_R`, counted. Its
  variable is no slot of its own: the loop holds its list or map ([#667]),
  and no change to it goes ahead while the loop is over it ([#715]).
- `IN_GEN_ITER_FROM_REF` counts its container ([#667]) when the loop is
  over a variable, with an `arg_size` of 0. Over a value the stack holds,
  as a struct's field or a global is, its `arg_size` is 1, and the value
  `IN_REF_FROM_BACK` points at is a counted reference: the iterator takes
  that reference over, and counts nothing. Either way the iterator holds
  one reference, which `IN_GEN_ITER_UNWIND` and `IN_GEN_ITER_DESTROY`
  release, and `colm_list_iter_destroy()` pops the word without releasing
  it.
- A loop over a weak slot, a local, a parameter or an attribute declared
  `weak<list<T>>` or `weak<map<K, V>>`, takes the temporary's path too, as
  a loop over a struct's field does. `LangVarRef::canTakeRef()` gives false
  for any `weak()` slot, so `evaluateArgs()` reads it with its weak read,
  `FN_GET_LOCAL_WEAK_R` for a local or a parameter and
  `FN_GET_FIELD_WEAK_R` for an attribute, which pushes the target,
  counted, or nil. `IN_REF_FROM_BACK` then points at that, and
  `IN_GEN_ITER_FROM_REF`'s `arg_size` is 1. Given the slot itself, the
  iterator would walk the slot's cell as a list. A nil or freed target
  stops the loop, with `for loop over a nil list` or `map`. A weak field or
  global is read onto the stack already, through `FN_GET_STRUCT_WEAK_R`.
- A loop over a tree or a user iterator holds no struct: a user iterator
  yields only trees ([#456]).

### Calls and returns

- A function's argument is evaluated onto the stack, counted, and
  `IN_STASH_ARG` moves it into the call's argument slot. The callee's
  `release_locals()` releases it (above), and `IN_CLEAR_ARGS` pops the
  slots after the call, as today.
- A struct return value moves: `IN_SAVE_RET` pops it into the frame's
  `FR_RV`, `IN_RET` moves it to `exec->ret_val`, and `IN_LOAD_RETVAL`
  pushes it. None of them counts. A weak return value is a cell from
  `FN_SAVE_RET_WEAK`, which `FN_LOAD_RETVAL_WEAK` reads and frees once
  `IN_RET` has torn the frame down. So a struct that only the function's
  locals held is freed by then, and the caller gets nil ([#670]).
  `LangStmt::compile()`'s return and `Compiler::compileFunction()`'s nil
  return write `FN_SAVE_RET_WEAK` in a function that returns `weak<T>`,
  and `LangVarRef::evaluateCall()` writes `FN_LOAD_RETVAL_WEAK` after a
  call of one.
- `pop_call_frame()`, which `FN_EXIT` and a user iterator's teardown use,
  needs nothing for a struct. A frame it pops has saved no return value:
  only the return's unwind code, a jump and `IN_RET` follow `IN_SAVE_RET`.
  Its release of a tree return value, under `frame_info`'s `ret_tree`,
  never runs anyway, since `pdacodegen.cc` doesn't write `ret_tree`.
- A statement that drops a call's struct result ends with `IN_POP_PTR`.
- A host function (`T f( S: s ) = c_f`) follows the convention trees
  follow. It gets each struct argument as a counted reference, which it
  releases with `colm_struct_downref()`, as `test/colm.d/ext1.lm`'s host
  releases its `str` with `colm_tree_downref()`. It counts a struct it
  returns with `colm_struct_upref()`, unless it gives back an argument,
  whose reference it then passes on. The maintainer chose this, in
  planning #668, over the call releasing the arguments and counting the
  result for the host, and over refusing a struct in a host function.
  `Compiler::writeHostCall()` doesn't change. A host function can't take
  or give a weak reference ([#670]).
- The top level's return value is dropped today, unreleased, whether a
  tree or a struct ([#731]). A struct returned there is a lost struct until
  [#731] is fixed.

### Parsers, inputs and streams

- A send pops its parser or stream and pushes it back (4):
  `IN_SEND_TEXT_W`, `IN_SEND_TREE_W`, `IN_SEND_NOTHING`, `IN_SEND_EOF_W`,
  `IN_SEND_STREAM_W`, `IN_PARSE_FRAG_W`, `IN_REDUCE_COMMIT` and
  `IN_PRINT_TREE`. The stack's reference holds the parser while its
  actions run ([#667]). A send as a statement ends with `IN_POP_PTR`.
- `IN_SEND_STREAM_W`, and `IN_INPUT_PUSH_STREAM_WC` and `_WV`, release the
  stream they pop once `input_append_stream()` or `input_prepend_stream()`
  has counted it.
- `IN_SET_PARSER_CONTEXT` releases the context it pops once
  `colm_parser_set_context()` has counted it. `IN_SET_PARSER_INPUT` moves
  the input it pops into the parser, and releases the input the parser had
  ([#667]). Both push the parser back.
- A parse, `parse X: T [...]`, copies its parser with `IN_DUP_PTR` where
  `LangTerm::evaluateParse()` writes `IN_DUP_VAL` today. Each
  `IN_GET_PARSER_MEM_R` releases the parser it pops once it has counted
  the tree or the error it pushes. So a parse's parser is freed at the end
  of the parse, with what it holds, as is a parser a `finish` is called on
  that nothing else holds. `IN_GET_PARSER_STREAM` (`gets()`) counts the
  input it pushes, then releases the parser.
- `FN_EXIT` in an action releases the parser that `IN_PARSE_FRAG_W` left on
  the stack once `colm_parse_frag()` has let the parser finish up after the
  action. It pops the parser and uses it there, and a parse's parser may
  have no other reference.
- The instructions that pop an `_input` or a stream as their object, and
  don't push it back, release it: `IN_READ_REDUCE`, `IN_INPUT_PULL_WC` and
  `_WV`, `IN_INPUT_PUSH_WC` and `_WV`, `IN_INPUT_PUSH_IGNORE_WC` and `_WV`,
  `IN_INPUT_PUSH_STREAM_WC` and `_WV`, `IN_GET_COLLECT_STRING`, and
  `IN_STREAM_PULL_WC` and `_WV`. `IN_IINPUT_AUTO_TRIM_WC`,
  `IN_INPUT_AUTO_TRIM_WC` and `IN_INPUT_CLOSE_WC` push theirs back.

### Tests, comparisons and the stack

- A test of a struct takes only whether it is nil: `if` and
  `LangStmt::compileWhile()` write `IN_TST_NZ_PTR` before
  `IN_JMP_FALSE_VAL`, `!` before `IN_NOT_VAL`, and `&&` and `||` where a
  tree gets `IN_TST_NZ_TREE` (`LangExpr::evaluate()`). `S || T` then gives
  a `bool`, where today a short cut gives `S`'s address as an `int`, which
  differs from run to run, so no program can rely on it.
- A comparison of two structs, `==`, `!=`, `<`, `<=`, `>` or `>=`, writes
  `IN_PTRS_TO_VALS` before its `_VAL_` test. Both structs are alive as it
  takes their addresses, so two of them never compare equal, though either
  may be freed as it releases them.
- A statement that drops a struct ends with `IN_POP_PTR`, where
  `LangStmt::compile()`'s `ExprType` writes `IN_POP_VAL` today, and a
  capture copies one with `IN_DUP_PTR` (`LangTerm::evaluateCapture()`).
- The words an expression has under a call are in colm's record of them
  ([#744]), and the call's unwind code pops each as what it is, when a user
  iterator is torn down there or an `exit` unwinds through it. A struct
  among them is popped with `IN_POP_VAL` until this is built: its kind's
  pop, in `exprWordPop()` (`synthesis.cc`), becomes `IN_POP_PTR`.

## For [#669]

Each `_WV` above records what it changes, as today, and each `_BKT`
follows the rule: what it pushes is counted, and what it pops and keeps
nothing of is released. The records that keep a struct, which [#669] says
how to hold and release, are:

- `IN_SET_STRUCT_PTR_WV`'s old value, and `FN_SET_STRUCT_WEAK_WV`'s old
  cell;
- `IN_PTR_ACCESS_WV`'s pointer;
- the parser and the stream a send records, and `IN_PARSE_INIT_BKT`'s
  parser;
- the element a pop or a `remove` keeps ([#667]), and a `set`'s old value.

Today `IN_LOAD_INPUT_BKT` counts its input with `colm_tree_upref()`, and
`rcode_downref()` skips `IN_SET_STRUCT_VAL_BKT`'s word.

## Public interface

Installed headers change, and a program compiled by an older colm needs
recompiling, as after [#667]:

- `bytecode.h`: the 14 `IN_` opcodes and the 13 `FN_` codes above, at
  values no instruction has. No value changes.
- `pdarun.h`: `LI_Struct` and `LI_Weak`.
- `struct.h`: `colm_value_get()`, `colm_value_put()` and
  `colm_value_release()`, all exported.

A host function given a struct must release it, and one that returns a
struct must count it (above).

## Rejected

- **Borrowing a struct that is only the next instruction's object**,
  uncounted, so that the `_VAL_` instructions stay exactly as they are. A
  send or a `finish` runs the program's code with its parser on the stack,
  and a set or a `remove` that frees a struct leading back to its object
  frees the object under it. It also goes against (2), which counts every
  value the VM holds. The maintainer chose to count every struct on the
  stack, in planning #668.
- **Releasing a block's struct locals at the end of the block**, and **a
  host function that counts nothing**, the call releasing its arguments
  and counting its result, or **refusing structs in a host function**:
  the maintainer's choices above.
- **One set of instructions that tells a tree from a struct by its id** at
  run time: an `int` has no id, and [#710] keeps the kinds apart when colm
  compiles.
- **A list's and a map's instructions split by kind**: three times as
  many, where the runtime already has `value_type` and `key_type`.
- **Struct forms of each of the six comparisons**: `IN_PTRS_TO_VALS`
  serves all of them. Taking each operand's address as soon as it is
  evaluated would release the first before the second is made, which
  could be given the first's memory and compare equal to it.
- **The weak instructions in the one-byte space**: it has room for the
  strong ones, not for both.
- **A flag on `IN_GEN_ITER_FROM_REF` for a container the stack holds**:
  its `arg_size` says so already.

## Validation

When this is built, colm.d cases show, beside the requirements' and the
other designs' Validation, with `COLM_LEAK_CHECK` and `--valgrind` clean:

- A struct in each place, a local, a parameter, a struct's field, a
  global, a context's variable, a tree's attribute, a list's value, and a
  map's key and value, is freed when the place is overwritten or let go
  of: a weak reference to it reads nil right after.
- A struct that a statement drops, a `new` or a call's result, is freed at
  the statement, and a parse's parser at the end of the parse.
- `if`, `while`, `!`, `&&`, `||` and the six comparisons on structs give
  what they give on main, but for `||`'s `bool`, and lose nothing.
- A struct local is released on each route out of its frame: a return, a
  `break` out of nested loops, an `exit` from a nested call, a production
  action that rejects, and a user iterator left early. One declared in a
  block lives until its function returns.
- A loop over a list that a struct's field or a global holds loses
  nothing.
- A host function that releases its struct argument, and one that returns
  a struct it made, lose nothing.
- A weak reference in each place reads its target, and nil once it is
  freed. A weak parameter given a new struct reads nil.
- A weak token attribute that `make_token` gave a struct reads that
  struct, and still reads it once the program has dropped its other
  references to it.
- A loop over a weak list or map local, parameter or attribute reads its
  target's values, and stops on a nil or a freed target.

## Building it

This, [#667]'s runtime and [#669]'s backing up are built together, as
[#667] says: once a struct can be freed, every holder must count its
reference. Weak references come after, on [#670]'s compiler part, but for
the cell and its calls, which [#669]'s design builds with these for a
context object's hold. When this is built,
[the access instructions](access-instructions.md) gain the strong and the
weak instructions in their tables.

[#456]: https://github.com/adrian-thurston/colm-suite/issues/456
[#477]: https://github.com/adrian-thurston/colm-suite/issues/477
[#663]: https://github.com/adrian-thurston/colm-suite/issues/663
[#667]: https://github.com/adrian-thurston/colm-suite/issues/667
[#668]: https://github.com/adrian-thurston/colm-suite/issues/668
[#669]: https://github.com/adrian-thurston/colm-suite/issues/669
[#670]: https://github.com/adrian-thurston/colm-suite/issues/670
[#671]: https://github.com/adrian-thurston/colm-suite/issues/671
[#710]: https://github.com/adrian-thurston/colm-suite/issues/710
[#712]: https://github.com/adrian-thurston/colm-suite/issues/712
[#715]: https://github.com/adrian-thurston/colm-suite/issues/715
[#731]: https://github.com/adrian-thurston/colm-suite/issues/731
[#743]: https://github.com/adrian-thurston/colm-suite/pull/743
[#744]: https://github.com/adrian-thurston/colm-suite/issues/744
[#745]: https://github.com/adrian-thurston/colm-suite/issues/745
