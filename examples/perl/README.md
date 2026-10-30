# Experimental Perl target

The `--P5` target is an experimental byte-oriented implementation. The default
`-T0` emits per-state offsets and sorted transition ranges, target and action
tables, and a binary-search execution loop. This follows Ruby `-T0` in its
table-driven approach but uses a different layout: Ruby separates single
keys, ranges and default transitions and can use an indirection table.
Other output styles fail.
It is deliberately separate from the existing Ruby target, leaving room for
additional Perl output strategies.

The surrounding Perl must declare `$data`, `$p`, `$pe`, and `$cs`; `write data`
declares machine constants, `write init` initializes `$cs`, and `write exec`
consumes input from `$p` up to `$pe`. Offsets and length must be bytes; pass byte
strings and calculate lengths under `use bytes`. Actions are Perl source;
`fc` gives the numeric current byte and `fpc` its offset. Transition, to-state, from-state, and EOF actions are supported. When EOF
actions are present, the host must declare `$eof` and set it to the byte offset
of the actual end of input; use an undefined value for non-final chunks. Variables in actions follow their ordinary Perl lexical scope.
The same variables can be retained across calls for chunked input.

The target supports `fhold`, `fexec`, `fbreak`, `fentry`, `fnext`, `fgoto`,
`fcall`, and `fret` in non-EOF actions, including expression destinations.
`write exec noend` is supported; the caller must arrange for execution to stop. For call/return, the host must declare
`@stack` and `$top`; `write init` initializes `$top`. It rejects other control statements, EOF transitions,
scanners, access/getkey/stack overrides, and non-default styles. Variable
overrides for p, pe, eof, cs, and data support plain Perl host expressions. Semantic conditions
with Perl expressions are supported; Ragel control statements inside a
condition are rejected.
EOF actions using `fc` or control statements are also rejected, as are
`fgoto`, `fcall`, and `fret` in from-state actions.
It does not yet support exports or generated entry-point constants. The host
scanner currently reuses Ruby's limited source lexer: avoid complex Perl
quoting, heredocs and regex literals in and around Ragel blocks. It is not a
general Perl parser. Do not rely on this proof of concept for arbitrary Ragel
input. `use bytes` does not encode Unicode strings: encode input explicitly.

Build Ragel then run `sh examples/perl/check.sh`. The script generates and runs
both the Perl and Ruby versions of the same machine and compares their output.
The checked-in `greeting.pl` and `greeting.rb` show their generated code side by
side with trailing whitespace removed; the check verifies they are reproducible.
Inspect the generated output with:

```
ragel/ragel -L --P5 -o greeting.pl examples/perl/greeting.rl
ragel/ragel -L -R -o greeting.rb examples/perl/greeting-ruby.rl
```

## Runnable Perl examples

The `demos/` directory pairs each `.rl` source with its generated `.pl` and
an `.expected` output file:

- `product-code`: validate codes made of three uppercase letters and four digits.
- `key-value`: capture fields using transition actions.
- `streaming-greeting`: retain machine state while processing three input chunks.
- `login-events`: call ordinary Perl subroutines from actions to build an event.
- `sphinx-query`: tokenize a small subset of Sphinx extended search expressions.
- `sphinxql-select`: parse a SphinxQL SELECT and tokenize its MATCH expression.

Run a checked-in program with `perl examples/perl/demos/login-events.pl`.
Regenerate it after building Ragel with:

```sh
ragel/ragel -L --P5 -o examples/perl/demos/login-events.pl examples/perl/demos/login-events.rl
```

The checked-in examples have trailing whitespace removed for review; the test
normalizes whitespace before comparing generated code. `sh examples/perl/check.sh`
regenerates each example in a temporary directory and verifies its output. These
are demonstrations of the supported subset described above, not general log
parsers or complete Ruby parity.

## Linux::Event socket example

`demos/linux-event-ping.rl` and the generated `linux-event-ping.pl` combine
the Perl target with Linux::Event 0.117 or newer. A local Unix socket pair
sends `PING ALICE\r\n` in two event-loop turns. The Ragel machine keeps its
state across callbacks; after recognizing the complete request, the Stream
callback writes `PONG ALICE\r\n` back to the peer. The program verifies the
response and exits. This is a deliberately small, one-request demo protocol;
it is not an HTTP server or a production protocol implementation.

On Linux with Perl 5.36 or newer, a C toolchain and the other Linux::Event
prerequisites, install the module using `cpanm Linux::Event`, then run:

```sh
perl examples/perl/demos/linux-event-ping.pl
sh examples/perl/check-linux-event.sh
```

The integration check requires Linux::Event; the general `check.sh` remains
independent of that optional dependency. To regenerate the Perl source:

```sh
ragel/ragel -L --P5 -o examples/perl/demos/linux-event-ping.pl examples/perl/demos/linux-event-ping.rl
```

## HTTP-style request example

`demos/linux-event-http.rl` and its generated `.pl` demonstrate a small HTTP
request parser connected to `Linux::Event::IO::Sock::Stream`. The built-in
self-test sends requests through a local Unix socket pair, deliberately splits
the greeting request across callbacks, verifies the complete response bytes,
and covers 200, 404, and 400 outcomes. No listening socket is opened.

Supported format is deliberately narrow: `GET`, paths containing ASCII letters,
digits, slash, underscore or hyphen, `HTTP/1.1`, zero or more ASCII header
lines, CRLF terminators, and no request body. `/health` and `/hello/NAME`
respond with text; other accepted paths return 404. This does not implement
full HTTP, Host validation, persistent connections, pipelining, transfer
encodings, request bodies, or general security checks. Do not deploy this
self-test as an HTTP server.

With Linux::Event 0.117 or newer installed, run:

```sh
perl examples/perl/demos/linux-event-http.pl
sh examples/perl/check-linux-event.sh
```

To regenerate the Perl output after building Ragel:

```sh
ragel/ragel -L --P5 -o examples/perl/demos/linux-event-http.pl examples/perl/demos/linux-event-http.rl
```

## Sphinx search expression example

`demos/sphinx-query.rl` and its generated `.pl` recognize a small subset of
Sphinx extended query syntax: ASCII words, quoted phrases, `@field` limits,
implicit AND between terms, `|` for OR, and `-` or `!` for NOT. Field limits carry forward until
the next field limit. The output is a token stream, not an evaluated search or
an expression tree; Sphinx gives OR higher precedence than implicit AND.
This demo does not connect to Sphinx or use Linux::Event. It omits parentheses, escapes, Unicode, and other Sphinx operators.

Run the example with its built-in queries or pass your own query as an argument:

```sh
perl examples/perl/demos/sphinx-query.pl
perl examples/perl/demos/sphinx-query.pl '@title "state machine" perl | ruby'
ragel/ragel -L --P5 -o examples/perl/demos/sphinx-query.pl examples/perl/demos/sphinx-query.rl
```

See [Sphinx extended query syntax](https://sphinxsearch.com/docs/current/extended-syntax.html)
for the full language and its semantics.

## SphinxQL SELECT example

`demos/sphinxql-select.rl` and the generated `.pl` parse an illustrative
SphinxQL SELECT with column names or `*`, one index, a single-quoted MATCH
expression, and optional ORDER BY and LIMIT. It invokes the checked-in
`sphinx-query.pl` to tokenize the expression. Both programs must be in the
same directory. For example:

```sh
perl examples/perl/demos/sphinxql-select.pl
perl examples/perl/demos/sphinxql-select.pl "SELECT id FROM articles WHERE MATCH('perl') LIMIT 10;"
ragel/ragel -L --P5 -o examples/perl/demos/sphinxql-select.pl examples/perl/demos/sphinxql-select.rl
```

This is a deliberately limited parser demonstration, not a complete SphinxQL
or MySQL parser. It does not execute SQL, validate index or column names,
implement the MySQL wire protocol, authenticate clients, or connect to a
server. MATCH accepts raw ASCII text without SQL string escaping; syntax
outside the search-expression demo's subset is reported there as unsupported.
Other SQL clauses, aliases, functions, filters, multiple indexes, and SQL
string escapes are not implemented. It reports the order column but does not
interpret sort direction. See the [SphinxQL SELECT reference](https://sphinxsearch.com/docs/current/sphinxql-select.html).
