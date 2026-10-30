use strict;
use warnings;
use bytes;
use FindBin;

# Illustrative SphinxQL SELECT parser. No database connection is made.
%%{
    machine sphinxql_select;
    action column_char { $column .= chr(fc); }
    action save_column { push @columns, $column; $column = ''; }
    action index_char { $index .= chr(fc); }
    action query_char { $query .= chr(fc); }
    action order_char { $order .= chr(fc); }
    action limit_char { $limit .= chr(fc); }

    ws = [ \t\r\n]+;
    optws = [ \t\r\n]*;
    ident = (alpha | '_') (alnum | '_')*;
    column = ((ident | '*') $column_char) %save_column;
    columns = column (optws ',' optws column)*;
    index_name = ident $index_char;
    query_text = (any - ['])* $query_char;
    order_name = ident $order_char;
    row_count = digit+ $limit_char;
    main := optws 'SELECT'i ws columns ws 'FROM'i ws index_name
        ws 'WHERE'i ws 'MATCH'i optws '(' optws "'" query_text "'" optws ')'
        (ws 'ORDER'i ws 'BY'i ws order_name (ws ('ASC'i | 'DESC'i))?)?
        (ws 'LIMIT'i ws row_count)? optws ';'? optws;
}%%
%% write data;

my @statements = @ARGV ? @ARGV : (
    q{SELECT id, title FROM articles WHERE MATCH('@title "state machine" perl | ruby') ORDER BY id DESC LIMIT 10;},
    q{SELECT * FROM articles WHERE MATCH('@description database query') LIMIT 25;},
    q{SELECT id FROM articles WHERE MATCH('perl') LIMIT nope;},
);

for my $sql (@statements) {
    my $data = $sql;
    my ($p, $pe, $cs) = (0, length($data), 0);
    my ($column, $index, $query, $order, $limit) = ('', '', '', '', '');
    my @columns;
    %% write init;
    %% write exec;
    print "SQL: $sql\n";
    if ($cs < $sphinxql_select_first_final || $p != $pe) {
        print "  INVALID (outside supported SQL subset)\n";
        next;
    }
    print '  COLUMNS: ', join(', ', @columns), "\n";
    print "  INDEX: $index\n";
    print "  ORDER BY: $order\n" if $order ne '';
    print "  LIMIT: $limit\n" if $limit ne '';
    print "  MATCH expression tokens:\n";
    open my $parser, '-|', $^X, "$FindBin::Bin/sphinx-query.pl", $query
        or die "cannot run search-expression example: $!";
    while (my $line = <$parser>) {
        print "    $line";
    }
    close $parser or die "search-expression example failed: $?";
}
