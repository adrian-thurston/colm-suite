# Struct cycles with no weak reference: design

How the compiler meets item 14 of the
[requirements](struct-lifetime-requirements.md): a program that compiles
can't build a cycle of strong references. Part of [#663], for [#671]. A
check in `src/colm/synthesis.cc` refuses such a program when it compiles.
The check changes nothing in the bytecode, the reverse code or the
runtime, and needs nothing from reference counting ([#667] to [#669]), but
it relies on two other checks, which land before it (below). Of weak
references it needs only to know which places are declared weak: how one
is written is [#670]'s.

## The graph

The check works on types, not values: a node is a type of thing that can
hold a struct, and an edge says a thing of one type can hold one of
another strongly.

- **A struct node** for each `StructEl` in `Compiler::structEls`: a user
  struct, a context, `_input` and `stream`. The program's globals
  (`globalSel`) aren't one: nothing can hold them.
- **A generic node** for each `GenericType`: a `list`, a `map` or a
  `parser`, a `UniqueType` of `TYPE_GENERIC`. A list's or a map's element
  struct (`list_el_N`, `map_el_N`) is a node no edge leads to: a program
  can't name one, and the list or map that holds it has the edge to its
  value instead.
- **One tree node** for every tree type, `str`, `any` and `il` included.
  An `any` can be assigned to a variable of any tree type, unchecked
  (`2_00_types.adoc`), so a place that holds a tree, whatever its declared
  type, can hold a tree of any type. On main (d864f64e), a `str` field, a
  `list<str>` and an attribute of a def type all take a tree of another def
  through an `any`. A `cast` gives a def the children and attributes of
  another, and `make_token` puts its arguments into a token's attribute
  slots whatever their type. So trees aren't told apart.

## The edges

Each is a strong reference. A place declared weak gives no edge, and an
`int`, a `bool` or a `ref<T>` holds no reference and gives none.

- **A field** of a struct or a context (`ObjectField::StructFieldType` in
  its `ObjectDef`), to its type's node: a struct, a generic, or the tree
  node for any tree type. A context's field that holds a context is an
  edge, but no place ([What is rejected](#what-is-rejected)).
- **An attribute** of a def or a token (`ObjectField::UserFieldType` in a
  `LangEl`'s `objectDef`), from the tree node to its type's node, if a
  struct or a generic.
- **A `make_token` argument** after the id and the text, from the tree node
  to its type's node, if a struct or a generic
  (`LangTerm::evaluateMakeToken()`). The slot it goes into is known only
  when it runs, so it always counts as strong.
- **A list**, to its value's type (`GenericType::valueUt`). **A map**, to
  its key's type (`keyUt`), then its value's (`valueUt`). A key can't be
  weak.
- **A parser**, to the struct of its tree's context (`contextIn`), if it
  has one, then to `_input`, then to the tree node: what the runtime holds
  for it, its context, its input and its trees.
- **`_input`**, to the tree node, for the trees sent to it. **`stream`**
  holds no tree or struct and has none.

A local, a parameter, a return value or a global isn't a node: nothing
holds one, so no cycle runs through it.

## The struct types it trusts

A struct node trusts that a value declared of a struct type is of that
type. Elsewhere colm checks a struct's type where the struct is stored,
and a struct can't go into an `any` or be cast. But where a struct comes
out of a tree's attribute, its declared type doesn't bound it: a tree of
another type in the variable, through an `any`, or a `make_token`
argument in a slot of another type ([#477]), gives the read a struct of
whatever type the slot holds. On main (d864f64e), a `p` read back through
an attribute declared `q` can be put in its own field `V: q`, which then
holds it strongly: a cycle the graph has no edge for, which the check
would accept.

So the check relies on two others, which land before it. The first, from
review item 4 on [#741] ([#745]), checks the tree where an attribute is
read or set. Every instruction that reaches an attribute is given the id
of the object of the type that declares it, and stops the program through
`runtime_error()` when the tree's type has another object. A tree of the
right type keeps its own attributes in its slots, so this closes the `any`
route. The second is [#477]'s check of `make_token`'s arguments against
the token's attributes, which closes the other route. With both, every
struct a program can name has its declared type. The tree node's edges
still hold: a struct gets into a tree only through an attribute or a
`make_token` argument, each an edge out of the tree node whatever slot the
struct lands in.

## What is rejected

The check finds the strongly connected components of the graph, by
Tarjan's algorithm. It rejects each one with a struct or a generic node in
it that has more than one node, or a node with an edge to itself: these
are the struct types that can reach themselves through strong references.
The tree node alone isn't rejected: a cycle that runs only through trees is
outside [#663].

Every rejected component has a field, an attribute or a `make_token`
argument with both of its ends in it. A generic leads only to the types
written in it, and `_input` only to the tree node, so a cycle passes a
struct or a context the program declares, or the tree node, and each edge
out of those is one of these.

A component whose only places inside it would be contexts' fields that
hold contexts isn't rejected ([#779]): a context with a field of its own
type, as `Sub: ctx`, or two contexts that hold each other. Such a field is
an edge, which the components and the paths take, but not a place. A
context object has one holder (requirements 4 and 5 of the
[context ownership requirements](context-ownership-requirements.md)), so
such fields alone make chains of objects, never a cycle:

- nothing moves an object out of a field, a parameter or `this`, and a
  context object read out of a field is never a loan (requirement 14);
- an assignment notes the move of its right-hand side (`noteMove()`)
  before `assignValue()` loads the target's root, so a local put into a
  place reached through itself is refused at the target: `C->Sub = C`
  and `C->Sub->Sub = C` give `cannot use C: it was moved at 12:10`.

Only a context's field or a parser's context leads to a context, so any
other cycle through a context passes a parser, and has a place that
isn't such a field. It is rejected as before, at the first such place.

## The error

The places are put in order: the fields of each struct in `structEls`'
order, each in the order it is declared; the attributes of each def and
token in `Compiler::langEls`' order, each in the order it is declared; then
the `make_token` arguments in the order they are compiled.

Each rejected component gives one error, at its anchor: the first place in
that order with both ends in it. The error is at the anchor's location (its
`ObjectField::loc`, or the `make_token` call's) and names one cycle: the
anchor, then the shortest path back from the anchor's type to the anchor's
owner inside the component, found breadth first, each node's edges tried in
the order the edges above list them. The errors come in their anchors'
order. Each is one line, after its location: `strong references make a
cycle:`, a space, the steps separated by `, `, and `; declare a field or
attribute in it weak`. Each step is:

- a field: `S.F`, with `S` its struct's or context's name and `F` the
  field's;
- an attribute: `T.A`, with `T` its def's or token's name;
- `a make_token argument`;
- `a list's value`, `a map's key` or `a map's value`;
- `a parser's context`, `a parser's input` or `a parser's trees`;
- `an input's trees`.

A path can pass a context's field that holds a context, a step `S.F` like
any field's, as in `b.P, a parser's context, a.B`
(`test/colm.d/ctxcycle7.lm`).

A step out of the tree node is an attribute or a `make_token` argument of
any tree type: the tree before it can hold any tree.

## Where it runs

`Compiler::checkStrongCycles()` is the last thing `Compiler::compile()`
does, after `parsePatterns()`. It needs the `make_token` arguments, known
once `compileByteCode()` has compiled all the code:
`LangTerm::evaluateMakeToken()` keeps each one's type and location as it
compiles it. The passes after `compileByteCode()` refuse programs of their
own: `collectParserEls()` a pattern whose type isn't a def, `makeParser()`
a grammar `parse_stop` can't use, and `parsePatterns()` a pattern or a
constructor that doesn't parse. The check comes after all of them, and
runs only when no error came before it, so a program refused for another
reason prints what it prints now. It reports each error with `error()`,
then exits with status 1 once all are out.

## Examples

On main (d864f64e), the check rejects these, with:

- `doc/colm/code/struct_route.lm`, a stop holding the next:
  `6:2: strong references make a cycle: stop.Next; declare a field or
  attribute in it weak`
- `test/colm.d/sendown1.lm`, a context holding a parser of its own:
  `29:2: strong references make a cycle: ctx.P, a parser's context;
  declare a field or attribute in it weak`
- `test/colm.d/refcopy13.lm`, a def whose attribute is a `list<str>`:
  `14:2: strong references make a cycle: assign.L, a list's value; declare
  a field or attribute in it weak`
- `test/colm.d/lookup1.lm`, a struct with a `str` field that a token's
  attribute holds: `25:3: strong references make a cycle:
  lang_object.name, lookup_id.obj; declare a field or attribute in it
  weak`

## What changes when it lands

It lands after the two checks above, and with weak references or after
them, in an issue of its own: without weak references, a program it
rejects has no other way to be fixed. On main (d864f64e) it rejects 15
programs that compile, each of which then needs a weak place on each of
its cycles, as item 14 of the requirements lists:

- `doc/colm/code/struct_route.lm`;
- `lang_object` in `grammar/c++/c++.lm` and `test/colm.d/lookup1.lm`;
- `foo` in `test/colm.d/pointer1.lm`, whose field `Start` holds a tree,
  and whose def `start` holds a `foo` in its attribute `Foo`;
- the parser a context's field holds in `test/colm.d/accumbt2.lm`,
  `ctxset1.lm`, `ntrejbt5.lm`, `sendown1.lm` to `sendown4.lm`,
  `undofrag2.lm` and `undofrag3.lm`: the field is made weak, and the
  parser is held strongly in a local or a global;
- the `list<str>` attribute in `test/colm.d/globread2.lm` and
  `refcopy13.lm`.

`test/colm.d/globtake6.lm` has the same cycle as `globread2.lm` but is
refused first for another reason, and stays as it is. Ragel's, cgil's and
colm's own `.lm` files pass, as do the other manual examples and
grammars. In the manual, `2_02_list_map_struct_alias.adoc`'s "A field can
hold a struct of the same type." and `3_04_undo.adoc`'s
`+C->Inner = new parser<ctx::inner>( C )+` change too.

As built, for #766, on main (2d650535) it refuses 13 programs that
compile: `struct_route.lm`; `lang_object` in `lookup1.lm` (and
`grammar/c++/c++.lm`); `pointer1.lm`; the parser a context's field holds
in `accumbt2.lm`, `ctxhold3.lm`, `ctxset1.lm`, `ntrejbt5.lm`,
`undofrag2.lm` and `undofrag3.lm`; and the `list<str>` attribute in
`ctxset2.lm`, `globread2.lm`, `refcopy13.lm` and `treesend5.lm`.
`sendown1.lm` to `sendown4.lm` had been removed. The parser a context's
field holds can't be held strongly in a local or a global: [#706]'s setup
takes only a new object, `nil` or what the object holds, and refuses
taking the parser out. So the field is made weak and keeps the parser it
is given, which nothing else holds; once structs are freed, that needs
another answer. The check landed before [#477], so until it does, a
`make_token` argument put into an attribute of another struct type is a
route the graph has no edge for.

## Decided

- **A parser's reference to its context is strong.** Rejected: making it
  weak in the runtime, which frees a context only its parser holds at
  once, as in `+new parser<ctx::program>( new ctx() )+` in
  `3_06_send.adoc` and `test/colm.d/accum2.lm`; and leaving it out of the
  check, with the runtime dropping it when the parse finishes, which lets
  a parser that never finishes leak.
- **All trees are one node.** Rejected: telling trees apart by their
  productions and attribute types, with edges for `cast`, `make_tree`,
  `make_token` and `push_ignore`, which misses a cycle built by storing an
  `any` in a typed place ([#700] and [#726] check the type where the tree
  is used, not where it is stored); and leaving `str` out, which misses
  one built by storing an `any` in a `str`. Telling them apart later
  breaks no program: it only accepts more.
- **A struct's type is checked where it leaves a tree,** by the two checks
  above: the tree's type where an attribute is reached, and `make_token`'s
  arguments. Rejected: checking the struct's own type where
  `IN_GET_FIELD_VAL_R` reads it, since a pointer tree doesn't say whether
  its value is a struct or an `int`, so the check would fault on an `int`,
  and through a tree of another type the slot can be past the tree's last
  attribute; merging into the tree node every struct and generic type that
  a tree's attribute is declared as or a `make_token` argument has, closed
  over the types at the same field of any two, which rejects far more
  programs; and leaving a struct read out of a tree under another type
  outside item 14's guarantee.
- **A cycle of contexts' fields that hold contexts alone isn't
  rejected** ([#779]): the maintainer's answer. Requirement 4 of the
  context ownership requirements lets a context's field be of any
  context, and refuses `weak<T>` of one, so such a cycle had no way to
  compile. Rejected: allowing `weak<T>` of a context in a context's
  field, since compile time can't tell which object a weak reference
  holds, so outside code could reach a lent object through one
  (requirement 3); a rule that no context can hold a context that can
  reach it again, which accepts nothing new and rules out chains of
  contexts, such as one for each nested scope; and leaving such a field
  out of the graph altogether, which would also accept a cycle that
  passes a parser, and lean on the ownership check for parsers too.

## Validation

`COMP_ERR` cases in `test/colm.d`, for the issue that implements the
check, each with its error: a struct that holds itself through a field;
two that hold each other; one that holds itself through a list's value,
and one through a map's key; a context holding its own parser; a struct
with a `str` field that a def's attribute holds; a def whose attribute is a
`list<str>`; a struct that a `make_token` argument puts into a token; and a
program with two cycles, giving two errors in their anchors' order. Each
compiles and runs once a place on each of its cycles is weak. A program
with a cycle and a pattern that doesn't parse gives the pattern's error,
as it does now.

For the checks it relies on: a struct read out of a tree's attribute
through a tree of another type, from an `any`, and set as its own field,
stops at the read with `fatal:` and exit status 1, where before [#745] it
built the cycle (`attrtype3` in `test/colm.d`); and [#477]'s cases refuse a
`make_token` argument that doesn't fit the token's attribute.

For [#779]: `ctxcycle1` and `ctxcycle2` run a context that holds its own
type and two contexts that hold each other; `ctxcycle3` to `ctxcycle6` are
refused where a move would make an object hold itself; and `ctxcycle7`, a
cycle through a context's field and a parser, is refused at the parser's
field.

[#663]: https://github.com/adrian-thurston/colm-suite/issues/663
[#667]: https://github.com/adrian-thurston/colm-suite/issues/667
[#669]: https://github.com/adrian-thurston/colm-suite/issues/669
[#477]: https://github.com/adrian-thurston/colm-suite/issues/477
[#670]: https://github.com/adrian-thurston/colm-suite/issues/670
[#671]: https://github.com/adrian-thurston/colm-suite/issues/671
[#741]: https://github.com/adrian-thurston/colm-suite/pull/741
[#700]: https://github.com/adrian-thurston/colm-suite/issues/700
[#726]: https://github.com/adrian-thurston/colm-suite/issues/726
[#745]: https://github.com/adrian-thurston/colm-suite/issues/745
[#706]: https://github.com/adrian-thurston/colm-suite/issues/706
[#779]: https://github.com/adrian-thurston/colm-suite/issues/779
