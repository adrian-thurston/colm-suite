#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
"$root/ragel/ragel" -L --P5 -o "$tmp/greeting.pl" "$root/examples/perl/greeting.rl"
"$root/ragel/ragel" -L -R -o "$tmp/greeting.rb" "$root/examples/perl/greeting-ruby.rl"
sed 's/[[:blank:]]*$//' "$tmp/greeting.pl" > "$tmp/greeting-normalized.pl"
sed 's/[[:blank:]]*$//' "$tmp/greeting.rb" > "$tmp/greeting-normalized.rb"
diff -u "$root/examples/perl/greeting.pl" "$tmp/greeting-normalized.pl"
diff -u "$root/examples/perl/greeting.rb" "$tmp/greeting-normalized.rb"
grep -q 'my @_greeting_offsets =' "$tmp/greeting.pl"
grep -q 'while ($_ragel_lo <= $_ragel_hi)' "$tmp/greeting.pl"
perl -c "$tmp/greeting.pl"
perl "$tmp/greeting.pl" > "$tmp/perl.out"
ruby "$tmp/greeting.rb" > "$tmp/ruby.out"
diff -u "$tmp/ruby.out" "$tmp/perl.out"
printf 'ACCEPT:[ALICE\nFAIL:\nFAIL:\n' > "$tmp/expected"
diff -u "$tmp/expected" "$tmp/perl.out"
if "$root/ragel/ragel" --P5 -F0 -o "$tmp/unsupported.pl" "$root/examples/perl/greeting.rl" >"$tmp/err" 2>&1; then
    echo 'unsupported output style unexpectedly accepted' >&2
    exit 1
fi
grep -q 'Perl backend: unsupported code style' "$tmp/err"
"$root/ragel/ragel" --P5 -o "$tmp/stream.pl" "$root/examples/perl/stream.rl"
perl -c "$tmp/stream.pl"
perl "$tmp/stream.pl" > "$tmp/stream.out"
printf 'ACCEPT:ALICE\n' > "$tmp/stream.expected"
diff -u "$tmp/stream.expected" "$tmp/stream.out"
if "$root/ragel/ragel" --P5 -o "$tmp/unsupported.pl" "$root/examples/perl/unsupported.rl" >"$tmp/err" 2>&1; then
    echo 'unsupported action unexpectedly accepted' >&2
    exit 1
fi
grep -q 'Perl backend: unsupported Ragel control statements' "$tmp/err"
for pair in state-actions control next-hold call-ret goto conditions dynamic noend goto-dynamic call-ret-dynamic overrides table-ranges; do
    "$root/ragel/ragel" --P5 -o "$tmp/$pair.pl" "$root/examples/perl/$pair.rl"
    "$root/ragel/ragel" -R -o "$tmp/$pair.rb" "$root/examples/perl/$pair-ruby.rl"
    perl -c "$tmp/$pair.pl"
    perl "$tmp/$pair.pl" > "$tmp/$pair.perl"
    ruby "$tmp/$pair.rb" > "$tmp/$pair.ruby"
    diff -u "$tmp/$pair.ruby" "$tmp/$pair.perl"
done
printf 'FXTFXTE:ACCEPT\nFXT:FAIL\nF:FAIL\n:FAIL\n' > "$tmp/state.expected"
diff -u "$tmp/state.expected" "$tmp/state-actions.perl"
printf ':FAIL\n1:ACCEPT\n2:ACCEPT\n12:ACCEPT\n' > "$tmp/conditions.expected"
diff -u "$tmp/conditions.expected" "$tmp/conditions.perl"
printf 'position=1:ACCEPT\n' > "$tmp/next.expected"
diff -u "$tmp/next.expected" "$tmp/next-hold.perl"
printf 'CR:3:ACCEPT\n' > "$tmp/call.expected"
diff -u "$tmp/call.expected" "$tmp/call-ret.perl"
diff -u "$tmp/call.expected" "$tmp/call-ret-dynamic.perl"
printf 'J:2:ACCEPT\n' > "$tmp/goto.expected"
diff -u "$tmp/goto.expected" "$tmp/goto.perl"
diff -u "$tmp/goto.expected" "$tmp/goto-dynamic.perl"
printf 'ab!:2:ACCEPT\n' > "$tmp/overrides.expected"
diff -u "$tmp/overrides.expected" "$tmp/overrides.perl"
printf 'position=1:ACCEPT\n' > "$tmp/dynamic.expected"
diff -u "$tmp/dynamic.expected" "$tmp/dynamic.perl"
printf 'position=3\n' > "$tmp/control.expected"
diff -u "$tmp/control.expected" "$tmp/noend.perl"
diff -u "$tmp/control.expected" "$tmp/control.perl"
test "$(wc -l < "$tmp/table-ranges.perl")" -eq 16512
for demo in product-code key-value streaming-greeting login-events sphinx-query sphinxql-select; do
    "$root/ragel/ragel" -L --P5 -o "$tmp/$demo.pl" "$root/examples/perl/demos/$demo.rl"
    sed 's/[[:blank:]]*$//' "$tmp/$demo.pl" > "$tmp/$demo.normalized.pl"
    diff -u "$root/examples/perl/demos/$demo.pl" "$tmp/$demo.normalized.pl"
    perl -c "$tmp/$demo.pl"
    perl "$tmp/$demo.pl" > "$tmp/$demo.actual"
    diff -u "$root/examples/perl/demos/$demo.expected" "$tmp/$demo.actual"
done
printf 'Ruby comparisons and generated Perl examples passed.\n'
