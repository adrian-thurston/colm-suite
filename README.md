# Colm Suite

The Colm Suite is a unified distribution of two closely related language tools:

- **Colm** (COmputer Language Machinery): A programming language designed for the analysis and [transformation of computer languages](https://www.program-transformation.org/Transform/TransformationSystems). Influenced primarily by [TXL](https://www.txl.ca/).

- **Ragel**: A state machine compiler that generates executable finite state machines from regular expressions and state machine specifications.

The two projects share components and have a build dependency, so a unified
repository simplifies development and building. The significant links are:

- The **FSM library** (`src/libfsm/`) -- finite state machine construction and manipulation, used by both Colm and Ragel.
- The **code generation intermediate language** (`src/cgil/`) -- written in Colm, used by both projects for target language code generation.
- Ragel's **frontend is written in Colm** (`src/ragel/*.lm`), so Colm must build first to bootstrap Ragel.


## Colm

### What is a transformation language?

A transformation language has a type system based on formal languages.<br>
Rather than defining classes or data structures, one defines grammars.

A parser is constructed automatically from the grammar, and the parser is used for two purposes:

- to parse the input language,
- and to parse the structural patterns in the program that performs the analysis.

In this setting, grammar-based parsing is critical because it guarantees that both the input and the structural patterns are parsed into trees from the same set of types, allowing comparison.

### Features

- Colm's main contribution lies in the parsing method.<br>Colm's parsing engine is generalized, but it also allows for the construction of arbitrary global data structures that can be queried during parsing. In other generalized methods, construction of global data requires some very careful consideration because of inherent concurrency in the parsing method. It is such a tricky task that it is often avoided altogether and the problem is deferred to a post-parse disambiguation of the parse forest.
- By default Colm will create an elf executable that can be used standalone for that actual transformations.
- Colm is a static and strong typed scripting language.
- Colm is very tiny and fast and can easily be embedded/linked with c/cpp programs.
- Colm's runtime is a stackbased VM that starts with the bare minimum of the language and bootstraps itself.

### Examples

This is how Colm is greeting the world ([`hello_world.lm`](doc/colm/code/hello_world.lm)):
```colm
print "hello world\n"
```

Here's a Colm program implementing a little assignment language ([`assign.lm`](doc/colm/code/assign.lm)) and its parse tree synthesis afterwards.
```colm
lex
	token id / ('a' .. 'z' | 'A' .. 'Z' ) + /
	token number / ( '0' .. '9' )+ /
	literal `= `;
	ignore / [ \t\n]+ /
end

def value
	[id] | [number]

def assignment
	[id `= value `;]

def assignment_list
	[assignment assignment_list]
|	[assignment]
|	[]

parse Simple: assignment_list[ stdin ]

if ( ! Simple ) {
	print( "[error]\n" )
	exit( 1 )
}
else {
	for I:assignment in Simple {
		print( $I.id, "->", $I.value, "\n" )
	}
}
```

More real-world programs parsing several languages implemented in Colm can be found in the [`grammar/`](grammar/) folder.

### Colm usage

To immediately compile and run e.g. the `hello_world.lm` program from above, call

```
$ colm -r hello_world.lm
hello world
```

Run `colm --help` for help on further options.

```
$ colm --help
usage: colm [options] file
general:
   -h, -H, -?, --help   print this usage and exit
   -v --version         print version information and exit
   -b <ident>           use <ident> as name of C object encapulaing the program
   -o <file>            if -c given, write C parse object to <file>,
                        otherwise write binary to <file>
   -p <file>            write C parse object to <file>
   -e <file>            write C++ export header to <file>
   -x <file>            write C++ export code to <file>
   -m <file>            write C++ commit code to <file>
   -a <file>            additional code file to include in output program
   -E N=V               set a string value available in the program
   -I <path>            additional include path for the compiler
   -i                   activate branchpoint information
   -L <path>            additional library path for the linker
   -l                   activate logging
   -r                   run output program and replace process
   -c                   compile only (don't produce binary)
   -V                   print dot format (graphiz)
   -d                   print verbose debug information

```


## Ragel

Ragel compiles regular expressions and state charts to executable finite state
machines. The generated code can be output in a variety of host languages.

### Supported target languages

C, C++, D, Java, Ruby, C#, Go, OCaml, Rust, Julia, JavaScript, GNU ASM x86-64, and Crack.

### Code generation backends

| Flag | Style |
|------|-------|
| `-T0`, `-T1` | Table-driven |
| `-F0`, `-F1` | Flat table-driven |
| `-G0`, `-G1`, `-G2` | Goto-driven |

Language-specific binaries are also available: `ragel-c`, `ragel-go`, `ragel-rust`, etc.

See the [`examples/`](examples/) directory for sample Ragel programs.


## Building

### Dependencies

- make
- libtool
- gcc
- g++
- autoconf
- automake

For the documentation, install [`asciidoc`](https://asciidoctor.org/) and [`fig2dev`](https://github.com/getlarky/fig2dev) as well.

### Build instructions

```
$ ./autogen.sh
$ ./configure
$ make
$ make install
```

### Run-time dependencies

The colm program depends on GCC at runtime. It produces a C program as output,
then compiles and links it with a runtime library. The compiled program depends
on the colm library.

To find the includes and the runtime library to pass to GCC, colm looks at
`argv[0]` to decide if it is running out of the source tree. If it is, then the
compile and link flags are derived from `argv[0]`. Otherwise, it uses the install
location (prefix) to construct the flags.


## Testing

```
$ make check
```

Test suites are under `test/` with subdirectories for each component (`colm.d`, `ragel.d`, `aapl.d`, etc.).


## Syntax highlighting

There are vim syntax definition files [colm.vim](/colm.vim) and [ragel.vim](/ragel.vim).


## License

Colm and Ragel are free software under the MIT license.<br>
Please see the COPYING file for more details.
