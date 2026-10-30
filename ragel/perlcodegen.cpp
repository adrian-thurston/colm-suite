/*
 * Experimental Perl target, added October 2026.
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include "perlcodegen.h"
#include "ragel.h"

/* Target language and output style. */
extern CodeStyle codeStyle;

static void unsupported( const char *what )
{
	std::cerr << "Perl backend: unsupported " << what << "\n";
	std::exit( 1 );
}

int PerlCodeGen::errorState()
{
	return redFsm->errState ? redFsm->errState->id : -1;
}

int PerlCodeGen::firstFinal()
{
	return redFsm->firstFinState ? redFsm->firstFinState->id : redFsm->nextStateId;
}

void PerlCodeGen::finishRagelDef()
{
	if ( codeStyle != GenTables )
		unsupported( "code style (only -T0 is available)" );

	redFsm->sortByStateId();
	analyzeMachine();

	if ( hasLongestMatch )
		unsupported( "scanner/longest match" );
	if ( getKeyExpr || accessExpr || prePushExpr || postPopExpr || topExpr || stackExpr ||
		actExpr || tokstartExpr || tokendExpr )
		unsupported( "variable or access overrides" );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans )
			unsupported( "EOF transitions" );
		if ( st->eofAction ) {
			for ( GenActionTable::Iter a = st->eofAction->key; a.lte(); a++ )
				for ( GenInlineList::Iter i = *a->value->inlineList; i.lte(); i++ )
					if ( i->type == GenInlineItem::Char )
						unsupported( "fc in EOF actions" );
		}
	}

	for ( GenActionList::Iter a = actionList; a.lte(); a++ ) {
		if ( !a->numRefs() )
			continue;
		if ( a->numFromStateRefs ) {
			for ( GenInlineList::Iter i = *a->inlineList; i.lte(); i++ )
				if ( i->type == GenInlineItem::Goto || i->type == GenInlineItem::Call ||
					i->type == GenInlineItem::Ret || i->type == GenInlineItem::GotoExpr ||
					i->type == GenInlineItem::CallExpr )
					unsupported( "goto/call/return in from-state actions" );
		}
		if ( a->numEofRefs ) {
			for ( GenInlineList::Iter i = *a->inlineList; i.lte(); i++ )
				if ( i->type != GenInlineItem::Text && i->type != GenInlineItem::PChar )
					unsupported( "control statements in EOF actions" );
		}
		for ( GenInlineList::Iter i = *a->inlineList; i.lte(); i++ ) {
			if ( i->type != GenInlineItem::Text && i->type != GenInlineItem::Char &&
				i->type != GenInlineItem::PChar && i->type != GenInlineItem::Hold &&
				i->type != GenInlineItem::Next && i->type != GenInlineItem::Break &&
				i->type != GenInlineItem::Goto && i->type != GenInlineItem::Call &&
				i->type != GenInlineItem::Ret && i->type != GenInlineItem::Exec &&
				i->type != GenInlineItem::NextExpr && i->type != GenInlineItem::GotoExpr &&
				i->type != GenInlineItem::CallExpr && i->type != GenInlineItem::Entry )
				unsupported( "Ragel control statements in actions" );
		}
	}
}

void PerlCodeGen::writeData()
{
	if ( noPrefix || noEntry || noFinal || noError )
		unsupported( "write data options" );

	out << "my $" << fsmName << "_start = " << redFsm->startState->id << ";\n";
	out << "my $" << fsmName << "_first_final = " << firstFinal() << ";\n";
	out << "my $" << fsmName << "_error = " << errorState() << ";\n";

	std::map<RedAction *, int> actionIds;
	std::vector<long long> low, high;
	std::vector<int> targets, actions, offsets;

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		offsets.push_back( (int)low.size() );
		for ( RedTransList::Iter tr = st->outRange; tr.lte(); tr++ ) {
			low.push_back( tr->lowKey.getLongLong() );
			high.push_back( tr->highKey.getLongLong() );
			targets.push_back( tr->value->targ ? tr->value->targ->id : errorState() );
			RedAction *a = tr->value->action;
			if ( a && actionIds.find( a ) == actionIds.end() ) {
				const int id = (int)actionIds.size() + 1;
				actionIds[a] = id;
			}
			actions.push_back( a ? actionIds[a] : 0 );
		}
	}

	offsets.push_back( (int)low.size() );
	out << "my @_" << fsmName << "_offsets = (";
	for ( size_t i = 0; i < offsets.size(); i++ )
		out << offsets[i] << ",";
	out << ");\nmy @_" << fsmName << "_low = (";
	for ( size_t i = 0; i < low.size(); i++ )
		out << low[i] << ",";
	out << ");\nmy @_" << fsmName << "_high = (";
	for ( size_t i = 0; i < high.size(); i++ )
		out << high[i] << ",";
	out << ");\nmy @_" << fsmName << "_targets = (";
	for ( size_t i = 0; i < targets.size(); i++ )
		out << targets[i] << ",";
	out << ");\nmy @_" << fsmName << "_actions = (";
	for ( size_t i = 0; i < actions.size(); i++ )
		out << actions[i] << ",";
	out << ");\n";
}

void PerlCodeGen::writeExports()
{
	unsupported( "write exports" );
}

std::string PerlCodeGen::variable( GenInlineList *list, const char *fallback )
{
	if ( !list )
		return fallback;

	std::string value;
	for ( GenInlineList::Iter i = *list; i.lte(); i++ ) {
		if ( i->type != GenInlineItem::Text )
			unsupported( "Ragel expression in variable override" );
		value += i->data;
	}
	return value;
}

void PerlCodeGen::writeInit()
{
	if ( !noCS )
		out << variable( csExpr, "$cs" ) << " = " << redFsm->startState->id << ";\n";
	if ( redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "$top = 0;\n";
}

void PerlCodeGen::writeStart()
{
	out << redFsm->startState->id;
}

void PerlCodeGen::writeFirstFinal()
{
	out << firstFinal();
}

void PerlCodeGen::writeError()
{
	out << errorState();
}

void PerlCodeGen::action( RedAction *a )
{
	if ( !a )
		return;

	for ( GenActionTable::Iter item = a->key; item.lte(); item++ ) {
		for ( GenInlineList::Iter i = *item->value->inlineList; i.lte(); i++ ) {
			if ( i->type == GenInlineItem::Text )
				out << i->data;
			else if ( i->type == GenInlineItem::Char )
				out << "ord(substr(" << variable( dataExpr, "$data" ) << ", "
					<< variable( pExpr, "$p" ) << ", 1))";
			else if ( i->type == GenInlineItem::PChar )
				out << variable( pExpr, "$p" );
			else if ( i->type == GenInlineItem::Hold )
				out << "--" << variable( pExpr, "$p" ) << ";";
			else if ( i->type == GenInlineItem::Next )
				out << variable( csExpr, "$cs" ) << " = " << i->targState->id << ";";
			else if ( i->type == GenInlineItem::Break )
				out << "++" << variable( pExpr, "$p" ) << "; goto RAGEL_PERL_OUT;";
			else if ( i->type == GenInlineItem::Goto )
				out << variable( csExpr, "$cs" ) << " = " << i->targState->id
					<< "; goto RAGEL_PERL_AGAIN;";
			else if ( i->type == GenInlineItem::Call )
				out << "$stack[$top++] = " << variable( csExpr, "$cs" ) << "; "
					<< variable( csExpr, "$cs" ) << " = " << i->targState->id
					<< "; goto RAGEL_PERL_AGAIN;";
			else if ( i->type == GenInlineItem::Ret )
				out << variable( csExpr, "$cs" )
					<< " = $stack[--$top]; goto RAGEL_PERL_AGAIN;";
			else if ( i->type == GenInlineItem::Entry )
				out << i->targState->id;
			else if ( i->type == GenInlineItem::Exec ) {
				out << variable( pExpr, "$p" ) << " = (";
				expression( i->children );
				out << ") - 1;";
			} else if ( i->type == GenInlineItem::NextExpr ) {
				out << variable( csExpr, "$cs" ) << " = (";
				expression( i->children );
				out << ");";
			} else if ( i->type == GenInlineItem::GotoExpr ) {
				out << variable( csExpr, "$cs" ) << " = (";
				expression( i->children );
				out << "); goto RAGEL_PERL_AGAIN;";
			} else if ( i->type == GenInlineItem::CallExpr ) {
				out << "$stack[$top++] = " << variable( csExpr, "$cs" ) << "; "
					<< variable( csExpr, "$cs" ) << " = (";
				expression( i->children );
				out << "); goto RAGEL_PERL_AGAIN;";
			} else
				unsupported( "action item" );
		}
		out << "\n";
	}
}

void PerlCodeGen::expression( GenInlineList *list )
{
	for ( GenInlineList::Iter i = *list; i.lte(); i++ ) {
		if ( i->type == GenInlineItem::Text )
			out << i->data;
		else if ( i->type == GenInlineItem::Char )
			out << "ord(substr(" << variable( dataExpr, "$data" ) << ", " << variable( pExpr, "$p" )
				<< ", 1))";
		else if ( i->type == GenInlineItem::PChar )
			out << variable( pExpr, "$p" );
		else if ( i->type == GenInlineItem::Entry )
			out << i->targState->id;
		else
			unsupported( "Ragel statement in expression" );
	}
}

void PerlCodeGen::condition( GenAction *a )
{
	expression( a->inlineList );
}

void PerlCodeGen::writeExec()
{
	out << "{\n use bytes;\n while (";
	if ( !noEnd )
		out << variable( pExpr, "$p" ) << " < " << variable( peExpr, "$pe" ) << " && ";
	out << variable( csExpr, "$cs" ) << " != " << errorState() << ") {\n";
	if ( redFsm->anyFromStateActions() ) {
		out << "  my $_ragel_from = " << variable( csExpr, "$cs" ) << ";\n";
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st->fromStateAction ) {
				out << "  if ($_ragel_from == " << st->id << ") {\n";
				action( st->fromStateAction );
				out << "  }\n";
			}
		}
	}
	out << "  my $_ragel_key = ord(substr(" << variable( dataExpr, "$data" ) << ", "
		<< variable( pExpr, "$p" ) << ", 1));\n";
	if ( redFsm->anyConditions() ) {
		out << "  my $_ragel_original = $_ragel_key;\n";
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			for ( GenStateCondList::Iter c = st->stateCondList; c.lte(); c++ ) {
				GenCondSpace *space = c->condSpace;
				out << "  if (" << variable( csExpr, "$cs" ) << " == " << st->id
					<< " && $_ragel_original >= " << c->lowKey.getLongLong()
					<< " && $_ragel_original <= " << c->highKey.getLongLong() << ") {\n";
				out << "   $_ragel_key = " << space->baseKey.getLongLong()
					<< " + $_ragel_original - " << keyOps->minKey.getLongLong() << ";\n";
				for ( GenCondSet::Iter a = space->condSet; a.lte(); a++ ) {
					if ( a.pos() >= 55 )
						unsupported( "too many semantic conditions" );
					out << "   $_ragel_key += " << ( ( 1ULL << a.pos() ) * keyOps->alphSize() )
						<< " if (";
					condition( *a );
					out << ");\n";
				}
				out << "  }\n";
			}
		}
	}

	/* The frontend supplies sorted, disjoint transition ranges. A binary
	 * search in each state's slice selects the target and action table. */
	const std::string cs = variable( csExpr, "$cs" );
	out << "  my $_ragel_trans = -1;\n";
	out << "  if (" << cs << " >= 0 && " << cs << " < " << redFsm->nextStateId << ") {\n";
	out << "   my $_ragel_lo = $_" << fsmName << "_offsets[" << cs << "];\n";
	out << "   my $_ragel_hi = $_" << fsmName << "_offsets[" << cs << " + 1] - 1;\n";
	out << "   while ($_ragel_lo <= $_ragel_hi) {\n";
	out << "    my $_ragel_mid = int(($_ragel_lo + $_ragel_hi) / 2);\n";
	out << "    if ($_ragel_key < $_" << fsmName
		<< "_low[$_ragel_mid]) { $_ragel_hi = $_ragel_mid - 1; }\n";
	out << "    elsif ($_ragel_key > $_" << fsmName
		<< "_high[$_ragel_mid]) { $_ragel_lo = $_ragel_mid + 1; }\n";
	out << "    else { $_ragel_trans = $_ragel_mid; last; }\n";
	out << "   }\n  }\n";
	out << "  my $_ragel_action = 0;\n";
	out << "  if ($_ragel_trans >= 0) {\n";
	out << "   " << cs << " = $_" << fsmName << "_targets[$_ragel_trans];\n";
	out << "   $_ragel_action = $_" << fsmName << "_actions[$_ragel_trans];\n";
	out << "  } else { " << cs << " = " << errorState() << "; }\n";

	/* Number action tables in the same order as writeData. */
	std::map<RedAction *, int> actionIds;
	std::vector<RedAction *> orderedActions;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		for ( RedTransList::Iter tr = st->outRange; tr.lte(); tr++ ) {
			RedAction *a = tr->value->action;
			if ( a && actionIds.find( a ) == actionIds.end() ) {
				const int id = (int)actionIds.size() + 1;
				actionIds[a] = id;
				orderedActions.push_back( a );
			}
		}
	}
	for ( size_t i = 0; i < orderedActions.size(); i++ ) {
		out << "  if ($_ragel_action == " << ( i + 1 ) << ") {\n";
		action( orderedActions[i] );
		out << "  }\n";
	}
	if ( redFsm->anyActionGotos() || redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "  RAGEL_PERL_AGAIN: ;\n";
	if ( redFsm->anyToStateActions() ) {
		out << "  my $_ragel_to = " << variable( csExpr, "$cs" ) << ";\n";
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st->toStateAction ) {
				out << "  if ($_ragel_to == " << st->id << ") {\n";
				action( st->toStateAction );
				out << "  }\n";
			}
		}
	}
	out << "  last if " << variable( csExpr, "$cs" ) << " == " << errorState() << ";\n  ++"
		<< variable( pExpr, "$p" ) << ";\n }\n";
	if ( redFsm->anyEofActions() && !noEnd ) {
		out << " if (" << variable( pExpr, "$p" ) << " == " << variable( peExpr, "$pe" )
			<< " && defined(" << variable( eofExpr, "$eof" ) << ") && " << variable( pExpr, "$p" )
			<< " == " << variable( eofExpr, "$eof" ) << ") {\n";
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st->eofAction ) {
				out << "  if (" << variable( csExpr, "$cs" ) << " == " << st->id << ") {\n";
				action( st->eofAction );
				out << "  }\n";
			}
		}
		out << " }\n";
	}
	out << " RAGEL_PERL_OUT: ;\n}\n";
}
