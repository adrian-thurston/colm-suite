# Where a value lives, and the instructions that reach it

A value can live in a local, a global, a struct's field, a context's
variable, a tree's attribute or child, a list's end, a loop variable, a
`ref<T>` parameter, or a child or attribute named on its own in a `switch`
case. Each place has its own instructions for each access. These tables list
them as `src/colm` has them, for the struct reference designs ([#668],
[#669]) to extend. An issue in a cell is the one that last changed it.

## How an access is compiled

`LangVarRef::loadObj()` (`synthesis.cc`) loads what a name or a
qualification starts at, its root. `loadQualification()` loads each field
of the qualification with `loadField()`, and an assignment ends with
`setField()`, from `assignValue()`. Both take the instruction from the
field's `ObjectField`, which has a slot for each access:

| Slot | Access |
| --- | --- |
| `inGetR` | read a tree |
| `inGetWC`, `inGetWV` | load a tree to write inside it, in code that isn't undone, and in code that is |
| `inSetWC`, `inSetWV` | set a tree, not undone, and undone |
| `inGetValR`, `inGetValWC`, `inGetValWV`, `inSetValWC`, `inSetValWV` | the same, for a value: an `int` or a `bool` (`UniqueType::scalar()`), or a pointer to a struct, a list, a map or a parser (`ptr()`) |

The `ObjectField` constructor (`parsetree.h`) gives every slot `IN_HALT`.
`ObjectField::initField()` (`synthesis.cc`) sets the slots for each kind of
field, and the functions in `declare.cc` that make the built-in fields set
theirs. A slot left at `IN_HALT` stops the program when it runs, with
`fatal: IN_HALT -- compiler did something wrong`.

The parser undoes the code it backs up over: a production's action, a token
action, a preeof block, and a function or an iterator one of them calls.
`Compiler::revertOn` is set while colm compiles such code. A function or an
iterator is compiled twice, with it (`codeWV`) and without it (`codeWC`). A
write takes the `WV` slots when `revertOn` is set and the root isn't a local,
or the write goes through a pointer: a change to a local's tree isn't
undone. Each `WV` instruction records its `_BKT` in the reverse code. A load
records one that loads the same object again, and a set records the old
value and ends the unit (`rcode_unit_term()`). Backing up runs the unit: the
loads, then the set back to the old value. When the parse commits, the
`_BKT`s are freed, and the old values with them.

A load for writing splits the tree it loads: a tree that something else
shares (`refs` above 1) is copied, and the copy stored back where the tree
was (`split_tree()`). What shared the tree keeps it as it was. Without the
split, an attribute's set fails `colm_tree_set_field()`'s assertion that
the tree isn't shared, and a child's or a token's data's set changes the
tree for everything that shares it.

## Roots

| Root | Read | Write | Undone write, and its undo |
| --- | --- | --- | --- |
| A local, a parameter, `lhs`, `r1` to `rN`, a loop variable | the field's own slot, in the frame | the same | never undone |
| A global (`global`, `export`, `argv`, `arg0`, `stds`), also named as `a::G` | `IN_LOAD_GLOBAL_R` | `IN_LOAD_GLOBAL_WC` | `IN_LOAD_GLOBAL_WV`, `IN_LOAD_GLOBAL_BKT` |
| A context's variable, in the context's code | `IN_LOAD_CONTEXT_R` | `IN_LOAD_CONTEXT_WC` | `IN_LOAD_CONTEXT_WV`, `IN_LOAD_CONTEXT_BKT` |
| `this`, `input` | their own slots, below | the same | the same |
| What a pointer points to: `S->F`, `P->tree`, `L->head->F` | the pointer's read slot | the same: a pointer is only read | `IN_PTR_ACCESS_WV`, `IN_PTR_ACCESS_BKT` at the last pointer, then each field after it takes its `WV` slot |
| A child or attribute named on its own in a `switch` case | through the switch's variable | the same | as the switch's variable's ([#693]) |

For the last row, `loadObj()` puts the switch's variable in front of the
name. `LangVarRef::caseVar()` finds that variable, and the write is undone
(`isLocalRoot()`, in `assignValue()`) and changes a global
(`isGlobalRef()`, for [#714]'s check) when the variable's would. The
variable is named in the scope around the switch (`walkCaseClause()`,
`loadfinal.cc`), so a switch on a global or a context's variable compiles
([#692]).

## Fields: tree slots

| Field (`ObjectField::Type`) | `inGetR` | `inGetWC` | `inGetWV`, undo | `inSetWC` | `inSetWV`, undo |
| --- | --- | --- | --- | --- | --- |
| A local, a value parameter, `lhs`, `r1` to `rN` (`UserLocalType`, `ParamValType`, `LhsElType`, `RedRhsType`) | `IN_GET_LOCAL_R` | `IN_GET_LOCAL_WC`, splits (`get_local_split()`) | none | `IN_SET_LOCAL_WC` | none |
| A `ref<T>` parameter (`ParamRefType`) | `IN_GET_LOCAL_REF_R` | `IN_GET_LOCAL_REF_WC`, splits (`split_ref()`) | none | `IN_SET_LOCAL_REF_WC`, splits | none |
| A tree's attribute (`UserFieldType`) | `IN_GET_FIELD_TREE_R` | `IN_GET_FIELD_TREE_WC`, splits (`get_field_split()`) | `IN_GET_FIELD_TREE_WV`, `IN_GET_FIELD_TREE_BKT`, both split | `IN_SET_FIELD_TREE_WC` | `IN_SET_FIELD_TREE_WV`, `IN_SET_FIELD_TREE_BKT` |
| A tree's child (`RhsNameType`) | `IN_GET_RHS_VAL_R` | `IN_GET_RHS_VAL_WC`, splits (`get_rhs_el_split()`) | `IN_GET_RHS_VAL_WV`, `IN_GET_RHS_VAL_BKT`, both split | `IN_SET_RHS_VAL_WC` | `IN_SET_RHS_VAL_WV`, `IN_SET_RHS_VAL_BKT` |
| A struct's field, a global, a context's variable (`StructFieldType`) | `IN_GET_STRUCT_R` | `IN_GET_STRUCT_WC`, splits (`get_struct_split()`, [#628]) | `IN_GET_STRUCT_WV`, `IN_GET_STRUCT_BKT`, both split ([#628]) | `IN_SET_STRUCT_WC` | `IN_SET_STRUCT_WV`, `IN_SET_STRUCT_BKT` |
| A list or map element's links, a map element's key (`GenericElementType`, `GenericDependentType`) | as a struct's field | | | | |
| A token's `data` (`InbuiltFieldType`, `makeDataEl()`) | `IN_GET_TOKEN_DATA_R` | none | none | `IN_SET_TOKEN_DATA_WC` | `IN_SET_TOKEN_DATA_WV`, `IN_SET_TOKEN_DATA_BKT` |

No program names an element's links or key: [#699] took the element forms
away.

Every instruction that reads or sets a tree's attribute, as a tree or as a
value, and `IN_REF_FROM_QUAL_REF`, which takes a reference to one, is given
the id of the object of the type that declares the attribute, after the
attribute's offset. It stops the program when the tree's type has another
object (`lel_info`'s `object_type_id`): a tree of another type, which an
`any` can put in a variable of the type, keeps other attributes in the
slots, or has fewer ([#745]). The undos, and `IN_SET_FIELD_TREE_LEAVE_WC`,
which sets an attribute of the tree a constructor has just made, take no
id.

## Fields: value slots

| Field | `inGetValR` | `inGetValWC` | `inGetValWV` | `inSetValWC` | `inSetValWV`, undo |
| --- | --- | --- | --- | --- | --- |
| A local, a value parameter | `IN_GET_LOCAL_VAL_R` | `IN_GET_LOCAL_VAL_R` | `IN_GET_LOCAL_VAL_R` | `IN_SET_LOCAL_VAL_WC` | none |
| A tree's attribute | `IN_GET_FIELD_VAL_R` | none | none | `IN_SET_FIELD_VAL_WC` | `IN_SET_FIELD_VAL_WV`, `IN_SET_FIELD_TREE_BKT` ([#537]) |
| A struct's field, a global, a context's variable | `IN_GET_STRUCT_VAL_R` | `IN_GET_STRUCT_VAL_R` | `IN_GET_STRUCT_VAL_R` | `IN_SET_STRUCT_VAL_WC` | `IN_SET_STRUCT_VAL_WV`, `IN_SET_STRUCT_VAL_BKT` |

A tree holds a value attribute as a pointer tree (`colm_construct_pointer()`),
so setting one is undone as setting a tree attribute is. A `ref<T>`
parameter and a tree's child hold only trees.

## Built-in fields that are only read

| Field | Made by | Read |
| --- | --- | --- |
| A token's `file`, `line`, `col`, `pos` | `makeFileEl()`, `makeLineEl()`, `makeColEl()`, `makePosEl()` | `IN_GET_TOKEN_FILE_R`, `_LINE_R`, `_COL_R`, `_POS_R` |
| The `length` of a `str`, a list or a map | `addLengthField()` | `IN_STR_LENGTH`, `IN_LIST_LENGTH`, `IN_MAP_LENGTH` |
| `match_text`, `match_length`, in a token action | `addMatchText()`, `addMatchLength()` | `IN_GET_MATCH_TEXT_R`, `IN_GET_MATCH_LENGTH_R` |
| A stream's `tree` | `declareStreamField()` | `IN_GET_COLLECT_STRING` |
| `stdin`, `stdout`, `stderr`, a name given with `-D` | `addStdin()`, `addStdout()`, `addStderr()`, `addDefineArgs()` | `IN_GET_CONST`, in every get slot |
| `error` | `addError()` | `IN_GET_ERROR`, in `inGetR`, `inGetWC` and `inGetWV` |
| A list's `head`, `tail`, `top` | `initListFieldVal()` | `IN_GET_VLIST_MEM_R`, which takes a reference to a tree it reads ([#466]) |
| A parser's `tree`, `error` | `initParserField()` | `IN_GET_PARSER_MEM_R` |
| `this`, `input` | `addThis()`, `addInput()` | `IN_LOAD_CONTEXT_R`, `_WC`, `_WV` and `IN_LOAD_INPUT_R`, `_WC`, `_WV`, in the tree and value get slots |

All of them but `this` and `input` are const. `this` and `input` can't be
assigned either ([#617]).

## Loop variables

A loop variable's accesses are its iterator's (`IterImpl`, `synthesis.cc`).

| Loop | `inGetCurR` | `inGetCurWC` | `inSetCurWC` | `inRefFromCur` |
| --- | --- | --- | --- | --- |
| Over a tree: `triter`, `child`, `rev_child`, `repeat`, `rev_repeat`, `with_ignore` | `IN_TRITER_GET_CUR_R` | `IN_TRITER_GET_CUR_WC` | `IN_TRITER_SET_CUR_WC` | `IN_TRITER_REF_FROM_CUR` |
| A user iterator | `IN_UITER_GET_CUR_R` | `IN_UITER_GET_CUR_WC` | `IN_UITER_SET_CUR_WC` | `IN_UITER_REF_FROM_CUR` |
| Over a list's values, either way, or a map's values or keys | `IN_GEN_VITER_GET_CUR_R`, `IN_GEN_KITER_GET_CUR_R` | `IN_HALT`, refused | `IN_HALT`, refused | `IN_HALT`, refused |

None is undone: a loop over anything but a local's tree, or an attribute or
child of one, walks a copy that can't be written ([#711]).

## A `ref<T>` argument

| Argument | The parameter |
| --- | --- |
| A local, a value parameter, `lhs` | the variable: `IN_REF_FROM_LOCAL` |
| A `ref<T>` parameter | the caller's variable: `IN_REF_FROM_REF` |
| A loop variable | the loop's `inRefFromCur` |
| An attribute or child of one of those, with no pointer on the way | the attribute or child: `IN_REF_FROM_QUAL_REF` or `IN_RHS_REF_FROM_QUAL_REF`, after `loadQualificationRefs()` |
| A child named on its own in a case | the same, once `qualifyProdRef()` puts the switch's variable in front |
| A global, a struct's field, a context's variable, an attribute or child of one, anything through a pointer | a copy the function may only read, which `checkRefCopies()` checks ([#711]) |
| A token's `data` or `file`, `match_text` | the same kind of copy: `canTakeRef()` refuses a built-in field, so a loop over one walks a copy too, and yielding one is refused |
| A literal or another expression | a copy the function may change |

## Refused at compile time

- Setting a const field, `field "N" is const`, and writing through one,
  `left hand side qualification "N" is const`. A pointer after the const
  field lifts the second (`lookupQualification()`).
- Assigning `this` or `input`: `cannot assign to "this"`.
- Writing, assigning or taking a ref to the variable of a loop over a list
  or a map ([#607]).
- Changing a context object from outside its parse and setup ([#706]), and
  a global in what a restartable parser runs ([#714]).
- Writing through a `ref<T>` given a copy ([#711]).

## Slots left at `IN_HALT`

These are never emitted:

- The `WV` slots of a local, a value parameter, `lhs`, `r1` to `rN` and a
  `ref<T>` parameter: `loadLocalObj()` and `assignValue()` never undo a
  change to a local.
- `inGetValWC` and `inGetValWV` of a tree's attribute: nothing writes inside
  an `int` or a `bool`, and `loadQualification()` loads a pointer at or
  before the last one in the qualification with its read slot.
- `inGetWC` and `inGetWV` of a token's `data`: a `str` has nothing inside
  to write.
- The write slots of the fields that are only read: they are const, so a
  write to one, or through one, is refused.
- The value slots of a `ref<T>` parameter and of a tree's child.
- Every slot of a `LexSubstrType` field. Only `lexFactorLabel()`
  (`parser.cc`) makes one, and no grammar rule calls it.

## Still open

- [#480]: a switch on a qualified variable can't name the case's children
  on their own.
- [#491]: a case's children can't be named on their own inside a block in
  the case. A switch inside the case is such a block, so a switch on a
  case's child gives `cannot find name`.
- [#727]: a case's child named on its own can't be yielded.
- A function called on a case's attribute, as `Lst->push_tail( 4 )`, gives
  `cannot resolve qualification Lst`: `lookupMethodObj()` has no case
  branch, as `lookupObj()` has. Not filed.
- `stds` isn't const, as `argv` and `arg0` are, so a program can replace
  it. Not filed.
- [#724], [#725]: a write through a ref after a split can reach a tree that
  another variable shares.

[#466]: https://github.com/adrian-thurston/colm-suite/issues/466
[#480]: https://github.com/adrian-thurston/colm-suite/issues/480
[#491]: https://github.com/adrian-thurston/colm-suite/issues/491
[#537]: https://github.com/adrian-thurston/colm-suite/issues/537
[#607]: https://github.com/adrian-thurston/colm-suite/issues/607
[#617]: https://github.com/adrian-thurston/colm-suite/issues/617
[#628]: https://github.com/adrian-thurston/colm-suite/issues/628
[#668]: https://github.com/adrian-thurston/colm-suite/issues/668
[#669]: https://github.com/adrian-thurston/colm-suite/issues/669
[#692]: https://github.com/adrian-thurston/colm-suite/issues/692
[#693]: https://github.com/adrian-thurston/colm-suite/issues/693
[#699]: https://github.com/adrian-thurston/colm-suite/issues/699
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#711]: https://github.com/adrian-thurston/colm-suite/issues/711
[#714]: https://github.com/adrian-thurston/colm-suite/issues/714
[#724]: https://github.com/adrian-thurston/colm-suite/issues/724
[#725]: https://github.com/adrian-thurston/colm-suite/issues/725
[#727]: https://github.com/adrian-thurston/colm-suite/issues/727
[#745]: https://github.com/adrian-thurston/colm-suite/issues/745
