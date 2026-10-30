use strict;
use warnings;
use bytes;

# A deliberately small Sphinx extended-query syntax demonstration.
# Actions collect tokens; they are displayed only after the whole query matches.
%%{
    machine sphinx_query;
    action reset_field { $field = ''; }
    action field_char { $field .= chr(fc); }
    action word_char { $text .= chr(fc); }
    action phrase_char { $text .= chr(fc); }
    action finish_word { push @tokens, ['WORD', $field, $text]; $text = ''; }
    action finish_phrase { push @tokens, ['PHRASE', $field, $text]; $text = ''; }
    action not_token { push @tokens, ['NOT', '', chr(fc)]; }
    action or_token { push @tokens, ['OR', '', '|']; }

    ident = (alpha | digit | '_')+;
    field = '@' @reset_field (ident $field_char) ' '+;
    word = (ident $word_char) %finish_word;
    phrase = '"' ((alpha | digit | '_' | ' ')+ $phrase_char) '"' %finish_phrase;
    term = field? (('-' | '!') @not_token)? (word | phrase);
    main := ' '* term ((' '+ term) | (' '* '|' @or_token ' '* term))* ' '+;
}%%
%% write data;

my @queries = @ARGV ? @ARGV : (
    '@title "state machine" perl | ruby',
    '@body perl search',
    '@title articles @description database -obsolete',
    'perl |',
);
for my $query (@queries) {
    # The trailing space lets exit actions fire on the final token.
    my $data = $query . ' ';
    my ($p, $pe, $cs) = (0, length($data), 0);
    my ($field, $text) = ('', '');
    my @tokens;
    %% write init;
    %% write exec;
    print "QUERY: $query\n";
    if ($cs >= $sphinx_query_first_final && $p == $pe) {
        for my $token (@tokens) {
            my ($type, $scope, $value) = @$token;
            my $suffix = $scope ne '' ? " field=$scope" : '';
            print "  $type$suffix: $value\n";
        }
    }
    else {
        print "  INVALID (outside supported subset)\n";
    }
}
