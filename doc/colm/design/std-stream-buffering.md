# Buffering of stdout and stderr: requirements and design

For [#494] and its duplicate [#498]. This says what must be true of when
colm's `stdout` and `stderr` write out what a program sends them, and how
the runtime does it. Before #494 both were fully buffered, on a terminal too,
and written out only when the program's top-level code ended or it stopped,
so a line sent to `stderr` came out after `stdout` lines written later, and a
crash lost both.

## Terms

- `stdout` and `stderr` are the streams the runtime opens for a program on
  descriptors 1 and 2, `prg->stdout_val` and `prg->stderr_val`. A host's
  `stdout` and `stderr` are the C library's, with buffers of their own, and
  are not part of this.
- An *item* is one value a `print`, `prints` or `send` to a stream writes: a
  literal, a string, a number or a tree. Each is one `IN_PRINT_TREE`
  instruction.
- *Written out* means handed to the system with `write(2)`, so it reaches the
  descriptor even if the process dies right after.

## Requirements

1. What a program sends to `stdout` and `stderr` comes out in the order it
   sent it, wherever the two go: a terminal, or one pipe or file that both go
   to, as with `2>&1`.
2. An item sent to `stderr` is written out before the send ends. It survives
   the process dying right after, from a crash, a signal or `_exit`, and a
   host's output written after the send comes after it.
3. On a terminal, `stdout` is written out a line at a time, so a person sees
   each line as it is printed, and a crash loses at most a partial line.
4. On a pipe or a file, `stdout` is written out in blocks, as a file stream
   is, so a program that prints a great deal makes one write per block, not
   one per item or per line. What it still holds when the process dies is
   lost, as in C.
5. Printing a tree to either stream never makes a write per token or ignore.
   An item sent to `stderr` goes out in as few writes as the stream's buffer
   allows, after one that writes out what `stdout` held, if it held anything.
6. A write the system refuses stops the program, as for any stream
   (`fatal: could not write NAME: REASON`, status 1), at whichever send or
   flush writes it out. For `stdout`, that can be a send to `stderr` (1).
7. What `stdout` and `stderr` still hold is written out, as before, when the
   program's top-level code ends, when it stops on a run-time error, when an
   exit ends the process, and when `colm_delete_program` closes the streams.
8. It is the same with and without `fopencookie`, and it changes no
   installed header and no exported symbol.

## Design

- `stdout`: `colm_impl_new_fd()` (`src/colm/stream.c`) makes descriptor 1's
  stream line-buffered with `setvbuf( file, 0, _IOLBF, 0 )` when
  `isatty( 1 )`, right after opening it and before any I/O on it (3).
  Otherwise the C library's full buffering stays (4). The call is needed
  because a cookie stream has no descriptor behind it for the C library to
  test, so it would buffer fully on a terminal. With `fdopen()` it states
  what glibc does anyway (8).
- `stderr` keeps its full buffer. `IN_PRINT_TREE` (`src/colm/bytecode.c`)
  writes it out with `flush_stream()` after each item it sends to `stderr`
  (2, 5). It flushes per item, not per statement: each item is an
  instruction of its own, and a flush per statement would need a new
  instruction or operand.
- `IN_PRINT_TREE` writes out `stdout` before each item it sends to `stderr`
  (1), as C++ ties `std::cerr` to `std::cout`. The cost is one write of what
  `stdout` holds, at a send to `stderr` that follows output to `stdout`.
- A send is to `stderr` when its stream is `prg->stderr_val`, which
  `CONST_STDERR` always pushes. A stream the program opens itself on
  `/dev/stderr` is an ordinary file stream, and not covered.
- A failed flush stops the program through `data_flush_stream()`,
  `colm_stream_error()` and `runtime_error()`, which flushes both streams
  again, with `stopping` set so that a second failure is ignored (6).
- `flush_streams()` still runs at `FN_STOP`, in `runtime_error()` and in
  `exit_now()`, and `colm_delete_program` still closes the streams (7).
- Nothing is added to `struct stream_impl_data`: `input.h` is an installed
  header (8).

## Rejected

- `stderr` unbuffered (`_IONBF`), as in C: each token and ignore of a printed
  tree is then a write of its own, against 5. A 300,000-word tree sent to
  `stderr` made 600,001 writes and took 1.4 s on a terminal, against 0.4 s.
- `stderr` line-buffered: a message with no newline waits, against 2.
- `stdout` line-buffered everywhere: it would keep whole lines through a
  crash on a pipe, but costs a write per line, against 4. 300,000 lines
  through a pipe made 300,001 writes and took 0.9 s, against 184 writes and
  0.6 s.
- No tie between the two, as in C: on one pipe, `stderr` then comes out
  ahead of `stdout` written before it, against 1.
- Writing the streams out from a handler for a fatal signal: stdio isn't safe
  in a signal handler, and a library shouldn't install handlers in its
  host's process.

## History

- 7db01e42 (2012) added the `stdout` and `stderr` streams, which wrote each
  piece of a tree with `write(2)`: unbuffered.
- 3d8f3c6a (2015) opened them with `fdopen()`: "IO should be buffered for
  performance, we do many small prints when we iterate trees." That is the
  reason for 4 and 5. glibc line-buffers an `fdopen()` stream on a terminal
  and fully buffers it elsewhere, so `stderr` was then fully buffered on a
  pipe or a file.
- fe0e546c (2020) opened them with `fopencookie()`, so that closing one
  doesn't close the descriptor. With no descriptor to test, glibc buffered
  them fully on a terminal too. The commit is about leaking `FILE` structs
  and says nothing of buffering: the change of buffering was a side effect.
  A change to how these streams are opened must keep 1 to 8.

## Validation

- `test/colm.d/stdorder1.lm`: with both streams on one pipe, the program's
  own code writes to both in turn, a partial line and a tree among them (1,
  5); an exported function writes to both before the host ends with
  `_exit`, after output of its own (2); and a message with no newline sent
  to `stderr` comes out ahead of a line printed after it (2).
- `test/colm.d/ioerr2.lm`: `stdout` on `/dev/full` fails when the program
  ends, and at a send to `stderr` (6, 7); and `stderr` on `/dev/full` stops
  the program at the send to it (2, 6).
- `test/colm.d/stdtty1.lm`: the host gives the program a terminal of its
  own as its `stdout`, and a line the program prints reaches it while the
  program waits in a parse (3).
- By hand: the order and a killed program's output on a terminal
  (`script -qec`) and through a pipe (1, 2), and the writes a large tree
  sent to `stderr` makes, counted with `strace` (5).

## Out of scope

- [#445]: `stdout` written by an exported function, or before `exit`, still
  waits for the program's end or `colm_delete_program`.
- Writing out `stdout` before a read from `stdin`, which a prompt with no
  newline would need.
- A way for a program to flush a stream itself.

[#445]: https://github.com/adrian-thurston/colm-suite/issues/445
[#494]: https://github.com/adrian-thurston/colm-suite/issues/494
[#498]: https://github.com/adrian-thurston/colm-suite/issues/498
