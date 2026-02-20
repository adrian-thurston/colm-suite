# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with
code in this repository.

## Project Overview

This repository contains two closely related projects:

- **Colm** (COmputer Language Machinery): A programming language and compiler
  construction toolkit designed for analysis and transformation of computer
  languages. Written in C/C++, it generates C code that compiles to native
  executables.

- **Ragel**: A state machine compiler that generates executable finite state
  machines from regular expressions and state machine specifications. It targets
  12+ languages: C, C++, D, Java, Ruby, C#, Go, OCaml, Rust, Julia, JavaScript,
  GNU ASM x86-64, and Crack.

Colm bootstraps Ragel -- Ragel's parser is written in Colm (`src/ragel/*.lm`).

## Build Commands

```bash
# Initial setup (only needed once)
./autogen.sh
./configure

# Build the project (both Colm and Ragel)
make

# Run tests
make check

# Install
make install

# Clean build artifacts
make clean
```

## Development Commands

```bash
# Compile a Colm program
colm file.lm

# Compile and run immediately
colm -r file.lm

# Compile only (don't produce binary)
colm -c file.lm

# Generate dot format for visualization
colm -V file.lm

# Compile a Ragel state machine
ragel -o output.c input.rl

# Code generation backends: -T0 (table), -T1, -F0 (flat), -F1, -G0 (goto), -G1, -G2
ragel -T0 -o output.c input.rl

# Host language selection via language-specific binaries
ragel-c input.rl
ragel-go input.rl
ragel-rust input.rl
```

## Testing

```bash
# Run all tests
make check

# Run tests directly with parallel execution
./test/runtests

# Run specific test suites
cd test/colm.d && ../../test/runtests     # Colm tests
cd test/ragel.d && ../../test/runtests    # Ragel tests
cd test/aapl.d && ../../test/runtests     # AAPL tests
cd test/rlhc.d && ../../test/runtests     # RLHC tests
cd test/rlparse.d && ../../test/runtests  # Ragel parser tests
cd test/trans.d && ../../test/runtests    # Translation tests
```

## Code Architecture

### Colm

1. **Compiler Pipeline** (src/):
   - `parsedata.cc/h`: Main parser data structures
   - `parsetree.cc/h`: Parse tree construction
   - `synthesis.cc`: Code synthesis and generation
   - `bytecode.cc/h`: Bytecode generation for the VM
   - `pdarun.c`: Runtime parser driver

2. **Runtime System** (src/):
   - `colm.c`: Main runtime entry point
   - `tree.c`: Tree manipulation runtime
   - `input.c`: Input stream handling
   - `program.c`: Program execution runtime

3. **Code Generation** (src/):
   - `cgil/`: Colm sources for code generation intermediate language

### Ragel

1. **Core** (src/ragel/):
   - `inputdata.cc/h`: Central orchestrator managing the compilation pipeline
   - `parsedata.cc/h`: Manages parsing of Ragel state machine definitions
   - `reducer.cc/h`: Transforms parsed data into intermediate representations
   - `parsetree.cc/h`: Parse tree structures for Ragel

2. **Parser Specs** (src/ragel/):
   - `ragel.lm`: Main Ragel grammar (written in Colm)
   - `rlparse.lm`: Parser definitions
   - `rlreduce.lm`: Reduction rules

3. **Host Language Targets** (src/ragel/host-*/):
   - 12 directories: `host-asm/`, `host-c/`, `host-crack/`, `host-csharp/`,
     `host-d/`, `host-go/`, `host-java/`, `host-js/`, `host-julia/`,
     `host-ocaml/`, `host-ruby/`, `host-rust/`
   - Each contains language-specific parser grammar and code generation

4. **Ragel Compilation Pipeline**:
   - Input processing: read and preprocess `.rl` source files
   - Parsing: Colm-based parsers build parse trees
   - Reduction: transform parse trees into FSM intermediate representation
   - FSM construction: build and optimize finite state machine graphs
   - Code generation: generate target language code using selected backend

### Shared

- **libfsm** (src/libfsm/): Finite state machine library used by both Colm and Ragel

### Key Abstractions

- **Parse Trees**: Central data structure for language processing
- **Virtual Machine**: Stack-based VM for executing Colm transformations
- **Input Streams**: Abstraction for handling various input sources
- **Patterns**: Grammar-based pattern matching for transformations
- **FsmGraph**: Core finite state machine representation and manipulation

### File Extensions

- `.lm`: Colm language source files
- `.cgil`: Code generation intermediate language files
- `.rl`: Ragel state machine definition files

## Coding Conventions

- Use tabs for indentation
- Spaces for alignment after first non-whitespace character
- Maximum 100 characters per line
- No trailing whitespace
- Function blocks start on new line
- C-style comments for documentation
- C++ style comments for disabled code

## Important Notes

- The project uses GNU autotools (autoconf, automake, libtool)
- Colm is self-hosting -- it uses itself to build parts of the compiler
- Ragel's parsers are written in Colm, so Colm must build first
- The test suite is comprehensive and uses expected output comparison
- Runtime requires GCC as Colm generates C code
- The project uses both C++ and Colm extensively
- When modifying parsers, changes to `.lm` files require rebuilding
- Multiple Ragel code generation backends are available -- choose based on performance/size requirements
- The project includes example grammars for C++, Go, Python, Rust, and other languages in the `grammar/` directory
