/*
 * Copyright 2006-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdbool.h>
#include <iostream>

#include "compiler.h"

using namespace std;

static void writeLexJoin( ostream &, LexJoin * );
static void writeLexExpression( ostream &, LexExpression * );
static void writeLexTerm( ostream &, LexTerm * );
static void writeLexFactorAug( ostream &, LexFactorAug * );
static void writeLexFactorRep( ostream &, LexFactorRep * );
static void writeLexFactorNeg( ostream &, LexFactorNeg * );
static void writeLexFactor( ostream &, LexFactor * );

static void writeRegExpr( ostream &, RegExpr * );
static void writeReItem( ostream &, ReItem * );
static void writeReOrBlock( ostream &, ReOrBlock * );

/* Helper function to escape special characters in EBNF strings */
static void escapeEbnfString( ostream &out, const String &str )
{
	for ( const char *p = str.data; *p != 0; p++ ) {
		switch ( *p ) {
			case '\'': out << "\\'"; break;
			case '\\': out << "\\\\"; break;
			case '\n': out << "\\n"; break;
			case '\r': out << "\\r"; break;
			case '\t': out << "\\t"; break;
			default: out << *p; break;
		}
	}
}

/* Write out a language element name in EBNF format.
 * Handles repeat/list/opt elements by recursively outputting the
 * original element name with the appropriate EBNF suffix. */
static void writeEbnfName( ostream &out, LangEl *lel )
{
	/* Handle generated repeat/list/opt elements */
	if ( lel->repeatOf != 0 ) {
		/* Output the original element name with appropriate EBNF suffix */
		writeEbnfName( out, lel->repeatOf );
		if ( lel->isRepeat )
			out << "*";
		else if ( lel->isList )
			out << "+";
		else if ( lel->isOpt )
			out << "?";
		return;
	}
	
	if ( lel->isLiteral && lel->lit.length() > 0 ) {
		//out << "'";
		//escapeEbnfString( out, lel->lit );
		out << lel->lit;
		//out << "'";
	}
	else {
		out << lel->name;
	}
}

static void dumpLangEl( LangEl *lel )
{
    cerr
        << "LangEl"
        << " name=[" << lel->name << "]"
        << " type=" << lel->type
        << " repeat=" << lel->isRepeat
        << " list=" << lel->isList
        << " opt=" << lel->isOpt
        << " repeatOf=[";

    if ( lel->repeatOf != 0 )
        cerr << lel->repeatOf->name;

    cerr
        << "]"
        << " literal=" << lel->isLiteral
        << " prodCount=" << lel->prodList.length()
        << " tokenDef=" << (void *)lel->tokenDef
        << " termDup=" << (void *)lel->termDup
        << " rootDef=" << (void *)lel->rootDef
        << "\n";
}

/* Escape a literal for an EBNF single-quoted terminal. */
static void writeEbnfLiteral( ostream &out, const String &str )
{
	out << "'";

	for ( const char *p = str.data; *p != 0; p++ ) {
		switch ( *p ) {
			case '\'':
				out << "\\'";
				break;
			case '\\':
				out << "\\\\";
				break;
			case '\n':
				out << "\\n";
				break;
			case '\r':
				out << "\\r";
				break;
			case '\t':
				out << "\\t";
				break;
			default:
				out << *p;
				break;
		}
	}

	out << "'";
}

/* Output a Literal node. */
static void writeLiteral( ostream &out, Literal *literal )
{
	if ( literal == 0 )
		return;

	if ( literal->type == Literal::Number ) {
		out << literal->literal;
	}
	else {
		writeEbnfLiteral( out, literal->literal );
	}
}

static void writeLexLiteral( ostream &out, Literal *literal )
{
	if ( literal == 0 )
		return;

	out << literal->literal;
}

/* Output a character range. */
static void writeRange( ostream &out, Range *range )
{
	if ( range == 0 )
		return;

	out << "[";

	writeLiteral( out, range->lowerLit );
	out << "-";
	writeLiteral( out, range->upperLit );

	out << "]";
}

static void writeLexLiteralChar( ostream &out, Literal *literal )
{
	if ( literal == 0 )
		return;

	/*
	 * Literal::literal contains the Colm spelling, including the
	 * surrounding quotes. For a character range we want the character
	 * itself, not the quotes.
	 */
	const char *p = literal->literal.data;

	if ( *p == '\'' ) {
		p++;

		/* Skip the closing quote when writing the character. */
		if ( *p == '\\' ) {
			out << *p++;
			if ( *p != 0 )
				out << *p++;
		}
		else if ( *p != 0 ) {
			out << *p++;
		}
	}
	else {
		/* Numeric literal or unexpected representation. */
		out << literal->literal;
	}
}

static void writeLexRange( ostream &out, Range *range )
{
	if ( range == 0 )
		return;

	out << "[";

	writeLexLiteralChar( out, range->lowerLit );
	out << "-";
	writeLexLiteralChar( out, range->upperLit );

	out << "]";
}

/* Convert a Colm builtin lexical machine to a useful textual name. */
static const char *builtinName( BuiltinMachine builtin )
{
	switch ( builtin ) {
		case BT_Any:    return "ANY";
		case BT_Ascii:  return "ASCII";
		case BT_Extend: return "EXTEND";
		case BT_Alpha:  return "ALPHA";
		case BT_Digit:  return "DIGIT";
		case BT_Alnum:  return "ALNUM";
		case BT_Lower:  return "LOWER";
		case BT_Upper:  return "UPPER";
		case BT_Cntrl:  return "CNTRL";
		case BT_Graph:  return "GRAPH";
		case BT_Print:  return "PRINT";
		case BT_Punct:  return "PUNCT";
		case BT_Space:  return "SPACE";
		case BT_Xdigit: return "XDIGIT";
		case BT_Lambda: return "LAMBDA";
		case BT_Empty:  return "EMPTY";
	}

	return "UNKNOWN";
}

static void writeReData( ostream &out, const String &data )
{
	for ( const char *p = data.data; *p != 0; p++ ) {
		switch ( *p ) {
			case '\\':
				out << "\\\\";
				break;
			case '\n':
				out << "\\n";
				break;
			case '\r':
				out << "\\r";
				break;
			case '\t':
				out << "\\t";
				break;
			default:
				out << *p;
				break;
		}
	}
}

static void writeReChar( ostream &out, char c )
{
	switch ( c ) {
		case '\\':
			out << "\\\\";
			break;
		case ']':
			out << "\\]";
			break;
		case '[':
			out << "\\[";
			break;
		case '-':
			out << "\\-";
			break;
		case '^':
			out << "\\^";
			break;
		case '\n':
			out << "\\n";
			break;
		case '\r':
			out << "\\r";
			break;
		case '\t':
			out << "\\t";
			break;
		default:
			out << c;
			break;
	}
}

static void writeReItem( ostream &out, ReItem *item )
{
	if ( item == 0 )
		return;

	switch ( item->type ) {
		case ReItem::Data:
			/*
			 * ReItem::data is already the character data from the
			 * regular-expression parser. Escape it so that it remains
			 * readable in the generated EBNF.
			 */
			writeReData( out, item->data );
			break;

		case ReItem::Dot:
			out << ".";
			break;

		case ReItem::OrBlock:
			out << "[";
			if ( item->orBlock != 0 ) {
				/* ReOrBlock is handled by writeReOrBlock(). */
				writeReOrBlock( out, item->orBlock );
			}
			out << "]";
			break;

		case ReItem::NegOrBlock:
			out << "[^";
			if ( item->orBlock != 0 )
				writeReOrBlock( out, item->orBlock );
			out << "]";
			break;
	}
}

static void writeReOrItem( ostream &out, ReOrItem *item );

static void writeReOrBlock( ostream &out, ReOrBlock *block )
{
	if ( block == 0 )
		return;

	if ( block->type == ReOrBlock::RecurseItem ) {
		writeReOrBlock( out, block->orBlock );
		writeReOrItem( out, block->item );
	}
}

static void writeReOrItem( ostream &out, ReOrItem *item )
{
	if ( item == 0 )
		return;

	switch ( item->type ) {
		case ReOrItem::Data:
			escapeEbnfString( out, item->data );
			break;

		case ReOrItem::Range:
			/*
			 * This is the internal regex representation of:
			 *
			 *     lower-upper
			 *
			 * Keep it recognizable in the generated output.
			 */
			writeReChar( out, item->lower );
			out << "-";
			writeReChar( out, item->upper );
			break;
	}
}

static void writeRegExpr( ostream &out, RegExpr *expr )
{
	if ( expr == 0 )
		return;

	if ( expr->type == RegExpr::RecurseItem ) {
		writeRegExpr( out, expr->regExp );
		writeReItem( out, expr->item );
	}
}

static void writeLexFactor( ostream &out, LexFactor *factor )
{
	switch ( factor->type ) {
		case LexFactor::LiteralType:
			writeLexLiteral( out, factor->literal );
			break;

		case LexFactor::RangeType:
			writeLexRange( out, factor->range );
			break;

		case LexFactor::OrExprType:
			writeReItem( out, factor->reItem );
			break;

		case LexFactor::RegExprType:
			writeRegExpr( out, factor->regExp );
			break;

		case LexFactor::ReferenceType:
			if ( factor->varDef != 0 )
				out << factor->varDef->name;
			else
				out << "<null-reference>";
			break;

		case LexFactor::ParenType:
			out << "(";
			writeLexJoin( out, factor->join );
			out << ")";
			break;
	}
}

static void writeLexFactorNeg( ostream &out, LexFactorNeg *neg )
{
	switch ( neg->type ) {
		case LexFactorNeg::NegateType:
			out << "^";
			writeLexFactorNeg( out, neg->factorNeg );
			break;

		case LexFactorNeg::CharNegateType:
			out << "^";
			writeLexFactorNeg( out, neg->factorNeg );
			break;

		case LexFactorNeg::FactorType:
			writeLexFactor( out, neg->factor );
			break;
	}
}

static void writeLexFactorRep( ostream &out, LexFactorRep *rep )
{
	switch ( rep->type ) {
		case LexFactorRep::StarType:
			writeLexFactorRep( out, rep->factorRep );
			out << "*";
			break;

		case LexFactorRep::StarStarType:
			writeLexFactorRep( out, rep->factorRep );
			out << "**";
			break;

		case LexFactorRep::OptionalType:
			writeLexFactorRep( out, rep->factorRep );
			out << "?";
			break;

		case LexFactorRep::PlusType:
			writeLexFactorRep( out, rep->factorRep );
			out << "+";
			break;

		case LexFactorRep::ExactType:
			writeLexFactorRep( out, rep->factorRep );
			out << "{" << rep->lowerRep << "}";
			break;

		case LexFactorRep::MaxType:
			writeLexFactorRep( out, rep->factorRep );
			out << "{," << rep->upperRep << "}";
			break;

		case LexFactorRep::MinType:
			writeLexFactorRep( out, rep->factorRep );
			out << "{" << rep->lowerRep << ",}";
			break;

		case LexFactorRep::RangeType:
			writeLexFactorRep( out, rep->factorRep );
			out << "{"
			    << rep->lowerRep
			    << ","
			    << rep->upperRep
			    << "}";
			break;

		case LexFactorRep::FactorNegType:
			writeLexFactorNeg( out, rep->factorNeg );
			break;
	}
}

static void writeLexFactorAug( ostream &out, LexFactorAug *aug )
{
	writeLexFactorRep( out, aug->factorRep );
}

static void writeLexTerm( ostream &out, LexTerm *term )
{
	switch ( term->type ) {
		case LexTerm::ConcatType:
			writeLexTerm( out, term->term );
			out << " ";
			writeLexFactorAug( out, term->factorAug );
			break;

		case LexTerm::RightStartType:
			out << "(";
			writeLexTerm( out, term->term );
			out << " :> ";
			writeLexFactorAug( out, term->factorAug );
			out << ")";
			break;

		case LexTerm::RightFinishType:
			out << "(";
			writeLexTerm( out, term->term );
			out << " :>> ";
			writeLexFactorAug( out, term->factorAug );
			out << ")";
			break;

		case LexTerm::LeftType:
			out << "(";
			writeLexTerm( out, term->term );
			out << " <: ";
			writeLexFactorAug( out, term->factorAug );
			out << ")";
			break;

		case LexTerm::FactorAugType:
			writeLexFactorAug( out, term->factorAug );
			break;
	}
}

static void writeLexExpression( ostream &out, LexExpression *expr )
{
	switch ( expr->type ) {
		case LexExpression::OrType:
			out << "(";
			writeLexExpression( out, expr->expression );
			out << " | ";
			writeLexTerm( out, expr->term );
			out << ")";
			break;

		case LexExpression::IntersectType:
			out << "(";
			writeLexExpression( out, expr->expression );
			out << " & ";
			writeLexTerm( out, expr->term );
			out << ")";
			break;

		case LexExpression::SubtractType:
			out << "(";
			writeLexExpression( out, expr->expression );
			out << " - ";
			writeLexTerm( out, expr->term );
			out << ")";
			break;

		case LexExpression::StrongSubtractType:
			out << "(";
			writeLexExpression( out, expr->expression );
			out << " -- ";
			writeLexTerm( out, expr->term );
			out << ")";
			break;

		case LexExpression::TermType:
			writeLexTerm( out, expr->term );
			break;

		case LexExpression::BuiltinType:
			out << builtinName( expr->builtin );
			break;
	}
}

static void writeLexJoin( ostream &out, LexJoin *join )
{
	if ( join == 0 || join->expr == 0 ) {
		out << "<empty>";
		return;
	}

	writeLexExpression( out, join->expr );
}

/* Write out token definitions from lex blocks */
void Compiler::writeEbnfTokens()
{
    ostream &out = *outStream;
    bool hasTokens = false;

    for ( NamespaceList::Iter ns = namespaceList; ns.lte(); ns++ ) {
        if ( ns->tokenDefList.length() > 0 ) {
            hasTokens = true;
            break;
        }
    }

    if ( !hasTokens )
        return;

    out << "/* Lexical regions:\n";

    for ( RegionSetList::Iter rs = regionSetList; rs.lte(); rs++ ) {
		//if (strcmp(rs->loc.fileName, "-") == 0) continue;
        out << " * [region set " << rs->id << "]";

        if ( rs->loc.fileName != 0 )
            out << " at " << rs->loc.fileName;

        out << ":" << rs->loc.line << "\n";
    }

    out << " */\n\n";

    out << "/* Tokens (Colm lexical expressions):\n";

    for ( NamespaceList::Iter ns = namespaceList; ns.lte(); ns++ ) {
        for ( TokenDefListNs::Iter tok = ns->tokenDefList;
                tok.lte(); tok++ ) {

            out << " * " << tok->name
                << " [region set " << tok->regionSet->id << "]";

            if ( tok->isIgnore )
                out << " (ignored)";

            out << " ::= ";

            if ( tok->isLiteral ) {
                writeEbnfLiteral( out, tok->literal );
            }
            else if ( tok->join != 0 ) {
                writeLexJoin( out, tok->join );
            }
            else {
                out << "<no expression>";
            }

            out << "\n";
        }
    }

    out << " */\n\n";
}

/* Write out a production in EBNF format */
void Compiler::writeEbnfProduction( Production *prod )
{
	ostream &out = *outStream;
	
	/* Output production elements */
	for ( ProdEl *el = prod->prodElList->head; el != 0; el = el->next ) {
		if ( el != prod->prodElList->head )
			out << " ";
		writeEbnfName( out, el->langEl );
	}
}

/* Write out all productions for a non-terminal */
void Compiler::writeEbnfLangEl( LangEl *lel )
{
    ostream &out = *outStream;

    /* Skip compiler-generated repetition/list/optional elements. */
    if ( lel->isRepeat || lel->isList || lel->isOpt )
        return;

    /* Skip terminals. */
    if ( lel->type != LangEl::NonTerm )
        return;

    /* Skip the compiler's synthetic root. */
	if ( lel == rootLangEl )
    	return;

    /* Skip elements with no productions. */
    if ( lel->prodList.length() == 0 )
        return;

    out << lel->name << "\n";

    bool first = true;
    for ( LelProdList::Iter prod = lel->prodList;
          prod.lte(); prod++ ) {
        if ( first ) {
            out << "    ::= ";
            first = false;
        }
        else {
            out << "\n      | ";
        }

        writeEbnfProduction( prod );
    }

    out << "\n\n";
}

/* Main EBNF generation function */
void Compiler::writeEbnfFile()
{
	ostream &out = *outStream;
	
	//for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) dumpLangEl( lel );

	out << "/* EBNF grammar generated by Colm */\n";
	out << "/* Compatible with https://www.bottlecaps.de/rr/ui */\n\n";
	
	/* Output tokens */
	writeEbnfTokens();
	
	/* Output productions */
	out << "/* Productions */\n\n";
	
	/* Iterate through all language elements */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ )
		writeEbnfLangEl( lel );
}
