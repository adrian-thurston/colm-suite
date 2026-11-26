/*
 * Copyright 2007-2018 Adrian Thurston <thurston@colm.net>
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

#include <assert.h>
#include <stdbool.h>
#include <iostream>
#include "compiler.h"

using std::cout;
using std::cerr;
using std::endl;

bool isStr( UniqueType *ut )
{
	return ut->typeId == TYPE_TREE && ut->langEl != 0 && ut->langEl->id == LEL_ID_STR;
}

IterDef::IterDef( Type type )
: 
	type(type), 
	func(0)
{
}

IterDef::IterDef( Type type, Function *func )
:
	type(type),
	func(func)
{}

IterImpl::IterImpl( Type type ) : 
	type(type), 
	func(0),
	useFuncId(false),
	useSearchUT(false),
	useGenericId(false)
{
	switch ( type ) {
	case Tree:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_ADVANCE;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;

	case Child:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_NEXT_CHILD;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;

	case RevChild:
		inCreateWV = IN_REV_TRITER_FROM_REF;
		inCreateWC = IN_REV_TRITER_FROM_REF;
		inUnwind =   IN_REV_TRITER_UNWIND;
		inDestroy =  IN_REV_TRITER_DESTROY;
		inAdvance =  IN_REV_TRITER_PREV_CHILD;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;
	
	case Repeat:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_NEXT_REPEAT;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;

	case RevRepeat:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_PREV_REPEAT;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;
	
	/* A list or map iterator can't take a reference to the current value, set
	 * it or load it for writing: the compiler refuses a loop variable used
	 * that way. IN_HALT stops a program if it ever misses one. */
	case ListVal:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_LIST_ITER_ADVANCE;

		inGetCurR =    IN_GEN_VITER_GET_CUR_R;
		inGetCurWC =   IN_HALT;
		inSetCurWC =   IN_HALT;
		inRefFromCur = IN_HALT;
		useGenericId = true;
		break;

	case RevListVal:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_REV_LIST_ITER_ADVANCE;

		inGetCurR =    IN_GEN_VITER_GET_CUR_R;
		inGetCurWC =   IN_HALT;
		inSetCurWC =   IN_HALT;
		inRefFromCur = IN_HALT;
		useGenericId = true;
		break;


	case MapVal:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_MAP_ITER_ADVANCE;

		inGetCurR =    IN_GEN_VITER_GET_CUR_R;
		inGetCurWC =   IN_HALT;
		inSetCurWC =   IN_HALT;
		inRefFromCur = IN_HALT;
		useGenericId = true;
		break;

	/* A map's keys, in key order. */
	case MapKey:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_MAP_ITER_ADVANCE;

		inGetCurR =    IN_GEN_KITER_GET_CUR_R;
		inGetCurWC =   IN_HALT;
		inSetCurWC =   IN_HALT;
		inRefFromCur = IN_HALT;
		useGenericId = true;
		break;

	case WithIgnore:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_WIG_ADVANCE;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;
	case User:
		assert(false);
	}
}

IterImpl::IterImpl( Type type, Function *func ) : 
	type(type),
	func(func),
	useFuncId(true),
	useSearchUT(true),
	useGenericId(false),
	inCreateWV(IN_UITER_CREATE_WV),
	inCreateWC(IN_UITER_CREATE_WC),
	inUnwind(IN_UITER_UNWIND),
	inDestroy(IN_UITER_DESTROY),
	inAdvance(IN_UITER_ADVANCE),
	inGetCurR(IN_UITER_GET_CUR_R),
	inGetCurWC(IN_UITER_GET_CUR_WC),
	inSetCurWC(IN_UITER_SET_CUR_WC),
	inRefFromCur(IN_UITER_REF_FROM_CUR)
{}

IterDef *Compiler::findIterDef( IterDef::Type type, Function *func )
{
	IterDefSetEl *el = iterDefSet.find( IterDef( type, func ) );
	if ( el == 0 )
		el = iterDefSet.insert( IterDef( type, func ) );
	return &el->key;
}

IterDef *Compiler::findIterDef( IterDef::Type type )
{
	IterDefSetEl *el = iterDefSet.find( IterDef( type ) );
	if ( el == 0 )
		el = iterDefSet.insert( IterDef( type ) );
	return &el->key;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId )
{
	UniqueType searchKey( typeId );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, LangEl *langEl )
{
	UniqueType searchKey( typeId, langEl );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, langEl );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, IterDef *iterDef )
{
	UniqueType searchKey( typeId, iterDef );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, iterDef );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, StructEl *structEl )
{
	UniqueType searchKey( typeId, structEl );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, structEl );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, GenericType *generic )
{
	UniqueType searchKey( typeId, generic );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, generic );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, UniqueType *target )
{
	UniqueType searchKey( typeId, target );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, target );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

/* 0-based. */
ObjectField *ObjectDef::findFieldNum( long offset )
{
	/* Bounds check. */
	if ( offset >= fieldList.length() )
		return 0;

	int fn = 0;
	FieldList::Iter field = fieldList; 
	while ( fn < offset ) {
		fn++;
		field++;
	}

	return field->value;
}

/* Finds the first field by type. */
ObjectField *ObjectDef::findFieldType( Compiler *pd, UniqueType *ut )
{
	for ( FieldList::Iter f = fieldList; f.lte(); f++ ) {
		UniqueType *fUT = f->value->typeRef->resolveType( pd );
		if ( fUT == ut )
			return f->value;
	}
	return 0;
}


long sizeOfField( UniqueType *fieldUT )
{
	long size = 0;
	switch ( fieldUT->typeId ) {
	case TYPE_ITER:
		/* Select on the iterator type. */
		switch ( fieldUT->iterDef->type ) {
			/* A for that names no iterator is a tree loop until it is
			 * compiled, after the frame is placed, and may turn out to be
			 * over a list or a map. So its slot must hold either iterator. */
			case IterDef::Tree:
				size = sizeof(tree_iter_t) > sizeof(generic_iter_t) ?
						sizeof(tree_iter_t) : sizeof(generic_iter_t);
				size /= sizeof(word_t);
				break;

			case IterDef::WithIgnore:
			case IterDef::Child:
			case IterDef::Repeat:
			case IterDef::RevRepeat:
				size = sizeof(tree_iter_t) / sizeof(word_t);
				break;

			case IterDef::RevChild:
				size = sizeof(rev_tree_iter_t) / sizeof(word_t);
				break;

			case IterDef::RevListVal:
			case IterDef::MapKey:
				size = sizeof(generic_iter_t) / sizeof(word_t);
				break;

			case IterDef::User:
				/* User iterators are just a pointer to the user_iter_t struct. The
				 * struct needs to go right beneath the call to the user iterator
				 * so it can be found by a yield. It is therefore allocated on the
				 * stack right before the call. */
				size = 1;
				break;
		}
		break;
	case TYPE_REF:
		size = 2;
		break;
	case TYPE_GENERIC:
		size = 1;
		break;
	case TYPE_LIST_PTRS:
		size = COLM_LIST_EL_SIZE;
		break;
	case TYPE_MAP_PTRS:
		size = COLM_MAP_EL_SIZE;
		break;
	default:
		size = 1;
		break;
	}

	return size;
}

void ObjectDef::referenceField( Compiler *pd, ObjectField *field )
{
	field->beenReferenced = true;
}

/* The variable of a loop over a list or a map. Its iterator gives the values
 * or the elements as they are stored, and has no instructions to take a
 * reference to the current one, to set it, or to load it for writing. */
static bool genericLoopVar( ObjectField *el )
{
	return el->iterImpl != 0 && el->iterImpl->useGenericId;
}

UniqueType *LangVarRef::loadField( Compiler *pd, CodeVect &code, 
		ObjectDef *inObject, ObjectField *el, bool forWriting, bool revert ) const
{
	/* Ensure that the field is referenced. */
	inObject->referenceField( pd, el );

	/* Loaded for writing, something is written inside it. */
	if ( forWriting )
		el->refWrite = true;

	UniqueType *elUT = el->typeRef->uniqueType;

	if ( elUT->ptr() || elUT->scalar() || elUT->weak() ) {
		if ( forWriting ) {
			/* The instruction, depends on whether or not we are reverting. */
			if ( pd->revertOn && revert )
				code.append( el->inGetValWV );
			else
				code.append( el->inGetValWC );
		}
		else {
			/* Loading for writing */
			code.append( el->inGetValR );
		}
	}
	else {
		/* If it's a reference then we load it read always. */
		if ( forWriting ) {
			/* The instruction, depends on whether or not we are reverting. */
			if ( elUT->typeId == TYPE_ITER )
				code.append( el->iterImpl->inGetCurWC );
			else if ( pd->revertOn && revert )
				code.append( el->inGetWV );
			else
				code.append( el->inGetWC );
		}
		else {
			/* Loading something for reading */
			if ( elUT->typeId == TYPE_ITER )
				code.append( el->iterImpl->inGetCurR );
			else
				code.append( el->inGetR );
		}
	}

	if ( el->useGenericId )
		code.appendHalf( el->generic->id );

	/* The context instructions take a context struct's id. The field this,
	 * the only field they load, is the parse's context, whatever it is, so it
	 * gives 0. */
	if ( el->inGetR == IN_LOAD_CONTEXT_R )
		code.appendHalf( 0 );

	if ( el->useOffset() ) {
		/* Gets of locals and fields require offsets. Fake vars like token
		 * data and lhs don't require it. */
		code.appendHalf( el->offset );

		/* An attribute's instruction takes the id of the object of the
		 * type that declares it, and checks the tree has that object. */
		if ( el->type == ObjectField::UserFieldType )
			code.appendHalf( inObject->id );
	}
	else if ( el->isRhsGet() ) {
		/* Need to place the array computing the val. */
		code.append( el->rhsVal.length() );
		for ( Vector<RhsVal>::Iter rg = el->rhsVal; rg.lte(); rg++ ) {
			code.append( rg->prodEl->production->prodNum );
			code.append( rg->prodEl->pos );
		}
	}

	if ( el->isConstVal ) {
		code.appendHalf( el->constValId );

		if ( el->constValId == CONST_ARG ) {
			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( el->constValArg, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.appendWord( mapEl->value );
		}
	}

	/* If we are dealing with an iterator then dereference it. */
	if ( elUT->typeId == TYPE_ITER )
		elUT = el->typeRef->searchUniqueType;

	/* A weak reference is read as its target. */
	return elUT->readType();
}

/* The qualification must start at a local frame. There cannot be any pointer. */
long LangVarRef::loadQualificationRefs( Compiler *pd, CodeVect &code,
		NameScope *rootScope ) const
{
	long count = 0;

	/* Start the search from the root object. */
	NameScope *searchScope = rootScope;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field in the current qualification. */
		ObjectField *el = searchScope->findField( qi->data );
		if ( el == 0 )
			error(qi->loc) << "cannot resolve qualification " << qi->data << endp;

		if ( qi.pos() > 0 ) {
			if ( el->isRhsGet() ) {
				code.append( IN_RHS_REF_FROM_QUAL_REF );
				code.appendHalf( 0 );

				code.append( el->rhsVal.length() );
				for ( Vector<RhsVal>::Iter rg = el->rhsVal; rg.lte(); rg++ ) {
					code.append( rg->prodEl->production->prodNum );
					code.append( rg->prodEl->pos );
				}
			}
			else { 
				code.append( IN_REF_FROM_QUAL_REF );
				code.appendHalf( 0 );
				code.appendHalf( el->offset );
				code.appendHalf( searchScope->owningObj->id );
			}
		}
		else if ( el->iterImpl != 0 ) {
			if ( genericLoopVar( el ) ) {
				error(qi->loc) << "cannot take a reference to " << el->name <<
						", the variable of a loop over a list or a map" << endp;
			}
			code.append( el->iterImpl->inRefFromCur );
			code.appendHalf( el->offset );
		}
		else if ( el->typeRef->type == TypeRef::Ref ) {
			code.append( IN_REF_FROM_REF );
			code.appendHalf( el->offset );
		}
		else {
			code.append( IN_REF_FROM_LOCAL );
			code.appendHalf( el->offset );
		}

		UniqueType *elUT = el->typeRef->uniqueType;
		if ( elUT->typeId == TYPE_ITER )
			elUT = el->typeRef->searchUniqueType;
		
		assert( qi->form == QualItem::Dot );

		ObjectDef *searchObjDef = elUT->objectDef();
		searchScope = searchObjDef->rootScope;

		count += 1;
	}
	return count;
}

/* The entry of a root, or of a parser variable, or 0. */
Compiler::Own *Compiler::findOwn( ObjectField *var )
{
	for ( int i = 0; i < owns.length(); i++ ) {
		if ( owns[i].var == var )
			return &owns[i];
	}
	return 0;
}

void Compiler::removeOwn( ObjectField *var )
{
	for ( int i = owns.length() - 1; i >= 0; i-- ) {
		if ( owns[i].var == var )
			owns.remove( i );
	}
}

void Compiler::addOwn( ObjectField *var, Own::Kind kind, const InputLoc &loc,
		ObjectField *parser )
{
	removeOwn( var );
	Own own;
	own.var = var;
	own.kind = kind;
	own.loc = loc;
	own.parser = parser;
	own.field = false;
	owns.append( own );
}

/* A root is lent to the parser variable. */
bool Compiler::lentToParser( ObjectField *parser )
{
	for ( int i = 0; i < owns.length(); i++ ) {
		if ( owns[i].kind == Own::LentToParser && owns[i].parser == parser )
			return true;
	}
	return false;
}

/* Nothing can send to the parser the variable holds any more: it was given
 * another, or is out of scope. What it was lent is the code's again. */
void Compiler::releaseParser( ObjectField *parser )
{
	for ( int i = owns.length() - 1; i >= 0; i-- ) {
		if ( ( owns[i].kind == Own::LentToParser && owns[i].parser == parser ) ||
				( owns[i].kind == Own::Finished && owns[i].var == parser ) )
			owns.remove( i );
	}
}

static bool sameOwn( const Compiler::Own &a, const Compiler::Own &b )
{
	return a.var == b.var && a.kind == b.kind && a.parser == b.parser;
}

/* After an if, what either side did holds: a root lent or moved on either
 * side is lent or moved, and a parser finished on either can't be sent to,
 * and gives back only what both sides gave back. */
void Compiler::mergeOwns( const Vector<Own> &other )
{
	for ( int i = 0; i < other.length(); i++ ) {
		Own *own = findOwn( other[i].var );
		if ( own == 0 )
			owns.append( other[i] );
		else if ( !sameOwn( *own, other[i] ) ) {
			bool otherMoved = other[i].kind == Own::Moved ||
					other[i].kind == Own::MovedWithParser;
			bool ownMoved = own->kind == Own::Moved ||
					own->kind == Own::MovedWithParser;
			if ( otherMoved && !ownMoved )
				*own = other[i];
			else if ( !otherMoved && !ownMoved ) {
				/* Lent to one parser on one side, another on the other. */
				own->kind = Own::MovedWithParser;
				own->parser = 0;
			}
		}
	}
}

static void ownPlace( std::ostream &out, const InputLoc &loc )
{
	out << loc.line << ":" << loc.col;
}

/* The variable is declared in scope, or, if within, in a scope inside it. */
static bool declaredIn( ObjectField *var, NameScope *scope, bool within )
{
	for ( NameScope *s = var->scope; s != 0; s = within ? s->parentScope : 0 ) {
		if ( s == scope )
			return true;
	}
	return false;
}

/* The variables of a scope have ended: nothing can send to a parser one of
 * them holds any more, and what they were lent is the code's again. */
void Compiler::endScopeOwns( NameScope *scope, bool within )
{
	Vector<ObjectField*> ended;
	for ( int i = 0; i < owns.length(); i++ ) {
		Own &own = owns[i];
		if ( own.var != 0 && declaredIn( own.var, scope, within ) )
			ended.append( own.var );
		if ( own.kind == Own::LentToParser && declaredIn( own.parser, scope, within ) )
			ended.append( own.parser );
	}
	for ( int i = 0; i < ended.length(); i++ ) {
		releaseParser( ended[i] );
		removeOwn( ended[i] );
	}
}

/* A loop's body runs again after its end: what it lends, moves or finishes
 * of what is outside it can't be as the next pass expects. */
void Compiler::checkLoopOwns( const Vector<Own> &start, const InputLoc &loopLoc )
{
	for ( int i = 0; i < owns.length(); i++ ) {
		const Own &own = owns[i];
		bool same = false;
		for ( int j = 0; j < start.length(); j++ ) {
			if ( sameOwn( own, start[j] ) )
				same = true;
		}
		if ( same )
			continue;

		String name = own.var != 0 ? own.var->name : String( "the context object" );
		std::ostream &out = error( loopLoc ) << name;
		switch ( own.kind ) {
			case Own::LentToParse: case Own::LentToParser: case Own::LentToLoop:
				out << " is lent in the loop's body (";
				ownPlace( out, own.loc );
				out << "), and isn't back by its end" << endp;
				break;
			case Own::Moved: case Own::MovedWithParser:
				out << " is moved in the loop's body (";
				ownPlace( out, own.loc );
				out << "), and isn't given another object by its end" << endp;
				break;
			case Own::Finished:
				out << " is finished in the loop's body (";
				ownPlace( out, own.loc );
				out << "), and the next pass can't send to it" << endp;
				break;
		}
	}
}

/* A loop ends when its test fails, which can be before the body runs, or at a
 * break. So what follows it starts from the state the test left, merged with
 * the state at each break, less what the loop's own variables held. What the
 * end of the body left adds nothing: checkLoopOwns() refused that. */
void Compiler::leaveLoop( const Vector<Own> &test, NameScope *scope )
{
	Vector< Vector<Own> > breaks = breakOwns;
	owns = test;
	for ( int b = 0; b < breaks.length(); b++ ) {
		Vector<Own> after = owns;
		owns = breaks[b];
		if ( scope != 0 )
			endScopeOwns( scope, true );
		Vector<Own> atBreak = owns;
		owns = after;
		mergeOwns( atBreak );
	}
}

/* Other code runs: a call, a parse, a send to a parser, a finish, a user
 * iterator's loop or a yield. A parse's item after the first that runs other
 * code makes the parser restartable. */
void Compiler::countOtherCode()
{
	otherCodeRuns += 1;
}

/* A list, map, parser, stream or struct other than a context: a value that
 * code holding it could change, and that a context object can hold. */
static bool ctxHeldRef( UniqueType *ut )
{
	/* A weak reference is held as its target. */
	ut = ut->readType();

	if ( ut->typeId == TYPE_GENERIC )
		return true;
	if ( ut->typeId == TYPE_STRUCT && !ut->structEl->structDef->isContext )
		return true;
	return false;
}

/* A tree type that refTrees has, as far as it is found. Any can be any tree. */
static bool refTreeSoFar( Compiler *pd, LangEl *lel )
{
	if ( lel == pd->anyLangEl )
		return pd->refTrees.length() > 0;
	return pd->refTrees.find( lel ) != 0;
}

/* A tree type with an attribute, or a child, that can hold a list, map,
 * parser, stream or struct, as far as the tree types found so far tell. */
static bool treeReachesRef( Compiler *pd, LangEl *lel )
{
	if ( lel == pd->anyLangEl )
		return pd->refTrees.length() > 0;

	if ( lel->objectDef != 0 ) {
		for ( FieldList::Iter f = lel->objectDef->fieldList; f.lte(); f++ ) {
			UniqueType *ut = f->value->typeRef != 0 ?
					f->value->typeRef->uniqueType : 0;
			if ( ut == 0 )
				continue;
			if ( ctxHeldRef( ut ) )
				return true;
			if ( ( ut->typeId == TYPE_TREE || ut->typeId == TYPE_REF ) &&
					ut->langEl != 0 && refTreeSoFar( pd, ut->langEl ) )
				return true;
		}
	}

	for ( LelProdList::Iter prod = lel->prodList; prod.lte(); prod++ ) {
		for ( ProdElList::Iter pe = *prod->prodElList; pe.lte(); pe++ ) {
			if ( pe->langEl != 0 && refTreeSoFar( pd, pe->langEl ) )
				return true;
		}
	}
	return false;
}

/* A copy of a tree shares what its attributes hold. The tree types that can
 * hold, below them, what code holding it could change, are found once, by
 * adding the types that reach one until no more do. */
bool Compiler::treeHoldsRef( LangEl *lel )
{
	if ( !refTreesFound ) {
		refTreesFound = true;
		bool grew = true;
		while ( grew ) {
			grew = false;
			for ( LelList::Iter el = langEls; el.lte(); el++ ) {
				if ( refTrees.find( el ) == 0 && treeReachesRef( this, el ) ) {
					refTrees.insert( el );
					grew = true;
				}
			}
		}
	}
	return refTreeSoFar( this, lel );
}

/* The article a message puts before a type's name: an any, an item, a
 * holder. */
static const char *article( const String &name )
{
	char c = name.length() > 0 ? name.data[0] : 0;
	switch ( c ) {
		case 'a': case 'e': case 'i': case 'o': case 'u':
		case 'A': case 'E': case 'I': case 'O': case 'U':
			return "an";
	}
	return "a";
}

/* A tree whose attributes, or a tree's below it, can hold what ctxHeldRef()
 * counts: a copy of the tree shares it. */
static bool sharesRef( Compiler *pd, UniqueType *ut )
{
	return ( ut->typeId == TYPE_TREE || ut->typeId == TYPE_REF ) &&
			ut->langEl != 0 && pd->treeHoldsRef( ut->langEl );
}

/* What code holding it could change, and a context object or a global can
 * hold: what ctxHeldRef() counts, and a tree that shares such a value. */
static bool heldRef( Compiler *pd, UniqueType *ut )
{
	return ctxHeldRef( ut ) || sharesRef( pd, ut );
}

/* An argument of a call that heldRef() counts. A built-in's arguments are
 * values, whose types evaluateArgs() keeps. */
static bool argHeldRef( Compiler *pd, CallArgVect *args )
{
	if ( args == 0 )
		return false;
	for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
		if ( (*pe)->exprUT != 0 && heldRef( pd, (*pe)->exprUT ) )
			return true;
	}
	return false;
}

/* A for loop reads what it goes over in place if it is named, and the loop's
 * values aren't what could be changed. */
static bool forInPlace( Compiler *pd, UniqueType *searchUT, CallArgVect *args )
{
	return !heldRef( pd, searchUT ) && args != 0 && args->length() == 1 &&
			args->data[0]->expr->type == LangExpr::TermType &&
			args->data[0]->expr->term->type == LangTerm::VarRefType;
}

/* A tree goes into a parse as itself, not as a copy, when it is sent with
 * send_tree, parsed with parse_tree or pushed onto a parser's input. One that
 * shares what its attributes hold would let the program change what the
 * parser's actions change while the parser can back up over them. */
static void checkTreeIntoParse( Compiler *pd, LangExpr *expr, UniqueType *ut,
		const char *what, const char *where, const char *instead )
{
	if ( !sharesRef( pd, ut ) )
		return;

	/* An expression that is a single term has its location in the term. */
	const InputLoc &loc = expr->type == LangExpr::TermType ?
			expr->term->loc : expr->loc;
	error( loc ) << "cannot " << what << " " << article( ut->langEl->xmlTag ) <<
			" " << ut->langEl->xmlTag << ", which can hold a list, map, parser, "
			"stream or struct" << where << ": " << instead << endp;
}

/* The root the reference starts at, as the ownership state keys it: a local
 * variable or a parameter, or 0 for the running context, which this and the
 * context's variables named alone reach. False for a global, and for input. */
bool LangVarRef::ownRoot( Compiler *pd, ObjectField *&root ) const
{
	if ( isNspaceQual() )
		return false;

	/* A child named on its own in a case starts where the switch's variable
	 * does. */
	const LangVarRef *var = caseVar( pd );
	if ( var != 0 )
		return var->ownRoot( pd, root );

	const String &first = qual->length() > 0 ? qual->data[0].data : name;
	if ( isInbuiltObject() ) {
		ObjectField *field = scope->findField( first );
		UniqueType *ut = field->typeRef->uniqueType;
		if ( ut->typeId == TYPE_STRUCT && ut->structEl->structDef->isContext ) {
			root = 0;
			return true;
		}
		return false;
	}

	if ( isLocalRef() ) {
		root = scope->findField( first );
		return root != 0;
	}

	if ( isStructRef() ) {
		root = 0;
		return true;
	}

	return false;
}

/* A use of a root that is lent or moved: see the ownership state in
 * doc/colm/design/context-ownership-design.md. */
void LangVarRef::checkOwnedUse( Compiler *pd ) const
{
	ObjectField *root;
	if ( !ownRoot( pd, root ) )
		return;

	if ( root == 0 && pd->revertOn && pd->curBlock != 0 )
		pd->curBlock->namesContext = true;

	Compiler::Own *own = pd->findOwn( root );
	if ( own == 0 || own->kind == Compiler::Own::Finished )
		return;

	const String &use = qual->length() > 0 ? qual->data[0].data : name;
	const InputLoc &useLoc = qual->length() > 0 ? qual->data[0].loc : loc;
	std::ostream &out = error(useLoc) << "cannot use " << use << ": ";
	out << ( root == 0 ? "the context object" : "it" );
	switch ( own->kind ) {
		case Compiler::Own::LentToParse:
			out << " is lent to the parse (";
			ownPlace( out, own->loc );
			out << ") for its statement" << endp;
			break;
		case Compiler::Own::LentToParser:
			out << " is lent to " << own->parser->name << " (";
			ownPlace( out, own->loc );
			out << ") until " << own->parser->name << "->finish()" << endp;
			break;
		case Compiler::Own::LentToLoop:
			out << " is lent to " << own->iter << " (";
			ownPlace( out, own->loc );
			out << ") until its loop ends" << endp;
			break;
		case Compiler::Own::Moved:
			out << " was moved at ";
			ownPlace( out, own->loc );
			out << endp;
			break;
		case Compiler::Own::MovedWithParser:
			out << " went with the parser made at ";
			ownPlace( out, own->loc );
			out << endp;
			break;
		case Compiler::Own::Finished:
			break;
	}
}

/* Outside its parse's actions, what a context object holds that could be
 * changed is read in place, never taken out, unless inPlace says the use is
 * one of the few that read it there. A tree that shares such a value is
 * taken out by any use of it as a value. */
void LangVarRef::checkContextValue( Compiler *pd, const VarRefLookup &lookup,
		UniqueType *ut, bool inPlace ) const
{
	if ( lookup.firstCtxPart < 0 || !heldRef( pd, ut ) || inPlace )
		return;

	std::ostream &out = error(loc) << "cannot take ";
	if ( lookup.objMethod != 0 )
		out << "what " << name << " gives";
	else
		out << name;
	out << " out of " << qual->data[lookup.firstCtxPart].data <<
			", a context object" << endp;
}

/* The name, or the qualification, starts at a global: loadObj() loads it
 * with loadGlobalObj() or loadScopedObj(). */
bool LangVarRef::isGlobalRef( Compiler *pd ) const
{
	if ( isNspaceQual() )
		return true;

	/* A child named on its own in a case starts where the switch's
	 * variable does. */
	const LangVarRef *var = caseVar( pd );
	if ( var != 0 )
		return var->isGlobalRef( pd );

	return !isInbuiltObject() && !isLocalRef() && !isStructRef();
}

/* In revert code, a change to what a global holds, or a value taken out of
 * a global that code holding it could change, is kept with the block being
 * compiled. Compiler::checkRestartable() refuses it if a restartable parser
 * runs the block, and a take, or a change that puts a reference value in, if
 * a parse given a context does: see doc/colm/design/global-change-design.md. */
void LangVarRef::noteGlobalUse( Compiler *pd, GlobalUse::Kind kind,
		const String &what, bool putsRef ) const
{
	if ( !pd->revertOn || pd->curBlock == 0 || !isGlobalRef( pd ) )
		return;

	/* A child named on its own in a case is a child of the global the
	 * switch's variable starts at. */
	const LangVarRef *root = this;
	for ( const LangVarRef *var = caseVar( pd ); var != 0; var = var->caseVar( pd ) )
		root = var;

	GlobalUse use;
	use.kind = kind;
	use.loc = qual->length() > 0 ? qual->data[0].loc : loc;
	use.global = root->qual->length() > 0 ? root->qual->data[0].data : root->name;
	use.what = what;
	use.putsRef = putsRef;
	pd->curBlock->globalUses.append( use );
}

/* What goes into a context object through a context reference: a new object,
 * nil, or something read through the same root. Anything else could be held
 * elsewhere too, and changed from there while a parser the object is lent to
 * can back up. A tree that shares such a value is never new: a cons or a
 * parse can put into it what is held elsewhere. So the object gets one from
 * its parse's actions. */
static UniqueType *evaluateSetupValue( Compiler *pd, CodeVect &code, LangExpr *expr )
{
	if ( !pd->ctxStore )
		return expr->evaluate( pd, code );

	ObjectField *root = pd->ctxStoreRoot;
	String name = pd->ctxStoreName;
	pd->ctxStore = false;

	LangTerm *term = expr->type == LangExpr::TermType ? expr->term : 0;
	bool fresh = term != 0 && ( term->type == LangTerm::NewType ||
			term->type == LangTerm::NilType );
	bool through = false;
	ObjectField *valueRoot;
	if ( term != 0 && term->type == LangTerm::VarRefType &&
			term->varRef->ownRoot( pd, valueRoot ) )
		through = valueRoot == root;

	pd->ctxValueInPlace = through;
	UniqueType *ut = expr->evaluate( pd, code );
	pd->ctxValueInPlace = false;

	if ( sharesRef( pd, ut ) && !fresh && !through ) {
		const InputLoc &loc = term != 0 ? term->loc : expr->loc;
		error(loc) << "only nil or what " << name << " holds can go into " <<
				name << ": " << article( ut->langEl->xmlTag ) <<
				" " << ut->langEl->xmlTag << " can hold a list, map, parser, "
				"stream or struct" << endp;
	}

	if ( ctxHeldRef( ut ) && !fresh && !through ) {
		const InputLoc &loc = term != 0 ? term->loc : expr->loc;
		error(loc) << "only a new object, nil or what " << name <<
				" holds can go into " << name << endp;
	}

	pd->ctxStore = true;
	pd->ctxStoreRoot = root;
	pd->ctxStoreName = name;
	return ut;
}

/* A store through lookup goes into a context object: note it for
 * evaluateSetupValue(). */
static void setCtxStore( Compiler *pd, const LangVarRef *varRef,
		const VarRefLookup &lookup )
{
	pd->ctxStore = false;
	if ( lookup.firstCtxPart >= 0 && varRef->ownRoot( pd, pd->ctxStoreRoot ) ) {
		pd->ctxStore = true;
		pd->ctxStoreName = varRef->qual->data[lookup.firstCtxPart].data;
	}
}

void LangVarRef::loadQualification( Compiler *pd, CodeVect &code, 
		NameScope *rootScope, int lastPtrInQual, bool forWriting, bool revert ) const
{
	/* Start the search from the root object. */
	NameScope *searchScope = rootScope;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field int the current qualification. */
		ObjectField *el = searchScope->findField( qi->data );
		if ( el == 0 )
			error(qi->loc) << "cannot resolve qualification " << qi->data << endp;

		if ( forWriting && el->refActive )
			error(qi->loc) << "reference active, cannot write to object" << endp;

		bool lfForWriting = forWriting;
		bool lfRevert = revert;

		/* If there is a pointer in the qualification, we need to compute
		 * forWriting and revert. */
		if ( lastPtrInQual >= 0 ) {
			if ( qi.pos() <= lastPtrInQual ) {
				/* If we are before or at the pointer we are strictly read
				 * only, regardless of the origin. */
				lfForWriting = false;
				lfRevert = false;
			}
			else {
				/* If we are past the pointer then we are always reverting
				 * because the object is global. Forwriting is as passed in.
				 * */
				lfRevert = true;
			}
		}

		if ( lfForWriting && genericLoopVar( el ) ) {
			error(qi->loc) << "cannot write to " << el->name <<
					", the variable of a loop over a list or a map" << endp;
		}

		UniqueType *qualUT = loadField( pd, code, searchScope->owningObj, 
				el, lfForWriting, lfRevert );
		
		if ( qi->form == QualItem::Dot ) {
			/* Cannot a reference. Iterator yes (access of the iterator not
			 * hte current) */
			if ( qualUT->ptr() )
				error(loc) << "dot cannot be used to access a pointer" << endp;
		}
		else if ( qi->form == QualItem::Arrow ) {
			if ( qualUT->ptr() ) {
				/* This deref instruction exists to capture the pointer reverse
				 * execution purposes. */
				if ( pd->revertOn && qi.pos() == lastPtrInQual && forWriting ) {
					/* This is like a global load. */
					code.append( IN_PTR_ACCESS_WV );
				}
			}
			else {
				error(loc) << "arrow operator cannot be used to "
						"access this type" << endp;
			}
		}

		ObjectDef *searchObjDef = qualUT->objectDef();
		searchScope = searchObjDef->rootScope;
	}
}

void LangVarRef::loadContextObj( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	/* Start the search in the global object. */
	ObjectDef *rootObj = structDef->objectDef;

	if ( forWriting && lastPtrInQual < 0 ) {
		/* If we are writing an no reference was found in the qualification
		 * then load the gloabl with a revert. */
		if ( pd->revertOn )
			code.append( IN_LOAD_CONTEXT_WV );
		else
			code.append( IN_LOAD_CONTEXT_WC );
	}
	else {
		/* Either we are reading or we are loading a pointer that will be
		 * dereferenced. */
		code.append( IN_LOAD_CONTEXT_R );
	}

	/* The id of the context's struct, which the parse's context must have. */
	code.appendHalf( structDef->structEl->id );

	loadQualification( pd, code, rootObj->rootScope, lastPtrInQual, forWriting, true );
}

void LangVarRef::loadGlobalObj( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	NameScope *scope = nspace != 0 ? nspace->rootScope : pd->rootNamespace->rootScope;

	if ( forWriting && lastPtrInQual < 0 ) {
		/* If we are writing an no reference was found in the qualification
		 * then load the gloabl with a revert. */
		if ( pd->revertOn )
			code.append( IN_LOAD_GLOBAL_WV );
		else
			code.append( IN_LOAD_GLOBAL_WC );
	}
	else {
		/* Either we are reading or we are loading a pointer that will be
		 * dereferenced. */
		code.append( IN_LOAD_GLOBAL_R );
	}

	loadQualification( pd, code, scope, lastPtrInQual, forWriting, true );
}

void LangVarRef::loadScopedObj( Compiler *pd, CodeVect &code, 
		NameScope *scope, int lastPtrInQual, bool forWriting ) const
{
//	NameScope *scope = nspace != 0 ? nspace->rootScope : pd->rootNamespace->rootScope;

	if ( forWriting && lastPtrInQual < 0 ) {
		/* If we are writing an no reference was found in the qualification
		 * then load the gloabl with a revert. */
		if ( pd->revertOn )
			code.append( IN_LOAD_GLOBAL_WV );
		else
			code.append( IN_LOAD_GLOBAL_WC );
	}
	else {
		/* Either we are reading or we are loading a pointer that will be
		 * dereferenced. */
		code.append( IN_LOAD_GLOBAL_R );
	}

	loadQualification( pd, code, scope, lastPtrInQual, forWriting, true );
}

void LangVarRef::loadInbuiltObject( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	/* Start the search in the local frame. */
	loadQualification( pd, code, scope, lastPtrInQual, forWriting, pd->revertOn );
}

void LangVarRef::loadLocalObj( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	/* Start the search in the local frame. */
	loadQualification( pd, code, scope, lastPtrInQual, forWriting, false );
}

void LangVarRef::loadObj( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	/* The root is read to reach what the qualification names. */
	if ( qual->length() > 0 )
		checkOwnedUse( pd );

	if ( isNspaceQual() )
		loadScopedObj( pd, code, qualNspace( pd )->rootScope, lastPtrInQual, forWriting );
	else if ( isInbuiltObject() )
		loadInbuiltObject( pd, code, lastPtrInQual, forWriting );
	else if ( isLocalRef() )
		loadLocalObj( pd, code, lastPtrInQual, forWriting );
	else if ( isProdRef( pd ) ) {
		LangVarRef *dup = new LangVarRef( *this );
		dup->qual->prepend( QualItem( QualItem::Dot, InputLoc(), scope->caseClauseVarRef->name ) );
		dup->loadObj( pd, code, lastPtrInQual, forWriting );
	}
	else if ( isStructRef() )
		loadContextObj( pd, code, lastPtrInQual, forWriting );
	else
		loadGlobalObj( pd, code, lastPtrInQual, forWriting );
}


bool castAssignment( Compiler *pd, CodeVect &code, UniqueType *destUT, 
		UniqueType *destSearchUT, UniqueType *srcUT )
{
	if ( destUT == srcUT )
		return true;

	/* Casting trees to any. */
	if ( destUT->typeId == TYPE_TREE && destUT->langEl == pd->anyLangEl &&
			srcUT->typeId == TYPE_TREE )
		return true;

	/* Setting a reference from a tree. */
	if ( destUT->typeId == TYPE_REF && srcUT->typeId == TYPE_TREE &&
			destUT->langEl == srcUT->langEl )
		return true;

	/* Setting a tree from a reference. */
	if ( destUT->typeId == TYPE_TREE && srcUT->typeId == TYPE_REF &&
			destUT->langEl == srcUT->langEl )
		return true;
	
	/* Setting an iterator from a tree. */
	if ( destUT->typeId == TYPE_ITER && srcUT->typeId == TYPE_TREE && 
			destSearchUT->langEl == srcUT->langEl )
		return true;
	
	/* Assigning nil to a tree. */
	if ( destUT->typeId == TYPE_TREE && srcUT->typeId == TYPE_NIL )
		return true;

	if ( destUT->typeId == TYPE_STRUCT && srcUT->typeId == TYPE_NIL )
		return true;

	if ( destUT->typeId == TYPE_GENERIC && srcUT->typeId == TYPE_NIL )
		return true;

	/* Setting a weak reference from its target type, or nil. */
	if ( destUT->typeId == TYPE_WEAK &&
			( srcUT == destUT->target || srcUT->typeId == TYPE_NIL ) )
		return true;

	if ( destUT->typeId == TYPE_TREE && srcUT->typeId == TYPE_TREE &&
			srcUT->langEl == pd->anyLangEl )
		return true;

	return false;
}

void LangVarRef::setFieldIter( Compiler *pd, CodeVect &code, 
		ObjectDef *inObject, ObjectField *el, UniqueType *objUT,
		UniqueType *exprType, bool revert ) const
{
	el->refWrite = true;

	code.append( el->iterImpl->inSetCurWC );
	code.appendHalf( el->offset );
}

void LangVarRef::setField( Compiler *pd, CodeVect &code, 
		ObjectDef *inObject, ObjectField *el,
		UniqueType *exprUT, bool revert ) const
{
	/* Ensure that the field is referenced. */
	inObject->referenceField( pd, el );

	el->refWrite = true;

	/* The field's type picks the store, not the value's: nil stored in a
	 * struct, list or map field replaces a pointer, which isn't released as
	 * a tree. */
	UniqueType *destUT = el->typeRef->uniqueType;
	if ( destUT->ptr() || destUT->scalar() || destUT->weak() ) {
		if ( pd->revertOn && revert )
			code.append( el->inSetValWV );
		else
			code.append( el->inSetValWC );
	}
	else {
		if ( pd->revertOn && revert )
			code.append( el->inSetWV );
		else
			code.append( el->inSetWC );
	}

	/* Maybe write out an offset. */
	if ( el->useOffset() ) {
		code.appendHalf( el->offset );
		if ( el->type == ObjectField::UserFieldType )
			code.appendHalf( inObject->id );
	}
	else if ( el->isRhsGet() ) {
		/* Need to place the array computing the val. */
		code.append( el->rhsVal.length() );
		for ( Vector<RhsVal>::Iter rg = el->rhsVal; rg.lte(); rg++ ) {
			code.append( rg->prodEl->production->prodNum );
			code.append( rg->prodEl->pos );
		}
	}
}


/* A parser variable lent an object is used only through ->, and sent to.
 * Used as a value, the parser can go anywhere, and what it was lent goes
 * with it for good. Once finished, it can't be sent to again. */
static void parserValueUse( Compiler *pd, ObjectField *var, const InputLoc &loc )
{
	Compiler::Own *fin = pd->findOwn( var );
	if ( fin != 0 && fin->kind == Compiler::Own::Finished ) {
		std::ostream &out = error(loc) << "cannot use " << var->name <<
				" as a value: it was finished at ";
		ownPlace( out, fin->loc );
		out << endp;
	}
	for ( int i = 0; i < pd->owns.length(); i++ ) {
		Compiler::Own &own = pd->owns[i];
		if ( own.kind == Compiler::Own::LentToParser && own.parser == var ) {
			if ( own.var == 0 || own.var->isParam() || own.field ) {
				std::ostream &out = error(loc) << "cannot use " << var->name <<
						" as a value: what it is lent (";
				ownPlace( out, own.loc );
				out << ") can't go with it" << endp;
			}
			if ( pd->throwaway == 0 ) {
				own.kind = Compiler::Own::MovedWithParser;
				own.parser = 0;
			}
		}
	}
}

UniqueType *LangVarRef::evaluate( Compiler *pd, CodeVect &code, bool forWriting ) const
{
	bool inPlace = pd->ctxValueInPlace;
	bool sendTarget = pd->sendTarget;
	pd->ctxValueInPlace = false;
	pd->sendTarget = false;

	/* Lookup the loadObj. */
	VarRefLookup lookup = lookupField( pd );

	/* A variable used as a value is read itself. */
	if ( qual->length() == 0 )
		checkOwnedUse( pd );

	/* Load the object, if any. */
	loadObj( pd, code, lookup.lastPtrInQual, forWriting );

	/* Load the field. */
	UniqueType *ut = loadField( pd, code, lookup.inObject, 
			lookup.objField, forWriting, false );

	checkContextValue( pd, lookup, ut, inPlace );

	if ( !inPlace && pd->revertOn && heldRef( pd, ut ) )
		noteGlobalUse( pd, GlobalUse::Take, qual->length() > 0 ? name : String() );

	if ( qual->length() == 0 && !sendTarget && isLocalRef() )
		parserValueUse( pd, lookup.objField, loc );

	return ut;
}

bool LangVarRef::canTakeRef( Compiler *pd, VarRefLookup &lookup ) const
{
	bool canTake = false;

	/* A token's data and file, and match_text, have no slot a ref can
	 * refer to. They are given as copies. */
	if ( lookup.objField->type == ObjectField::InbuiltFieldType )
		return false;

	/* If the var is not a local, it must be an attribute accessed
	 * via a local and attributes. */
	if ( lookup.inObject->type == ObjectDef::FrameType )
		canTake = true;
	else if ( isLocalRef() ) {
		if ( lookup.lastPtrInQual < 0 && ! lookup.uniqueType->ptr() ) 
			canTake = true;
	}

	return canTake;
}

void LangVarRef::verifyRefPossible( Compiler *pd, VarRefLookup &lookup ) const
{
	bool canTake = canTakeRef( pd, lookup );

	if ( !canTake ) {
		error(loc) << "can only take references of locals or "
				"attributes accessed via a local" << endp;
	}

	if ( lookup.objField->refActive )
		error(loc) << "reference currently active, cannot take another" << endp;
}

bool LangExpr::canTakeRef( Compiler *pd ) const
{
	bool canTake = false;

	if ( type == LangExpr::TermType && term->type == LangTerm::VarRefType ) {
		VarRefLookup lookup = term->varRef->lookupField( pd );
		if ( term->varRef->canTakeRef( pd, lookup ) )
			canTake = true;
	}

	return canTake;
}


/* Return the field referenced. */
ObjectField *LangVarRef::preEvaluateRef( Compiler *pd, CodeVect &code ) const
{
	VarRefLookup lookup = lookupField( pd );

	verifyRefPossible( pd, lookup );

	loadQualificationRefs( pd, code, scope );

	return lookup.objField;
}

/* Return the field referenced. */
ObjectField *LangVarRef::evaluateRef( Compiler *pd, CodeVect &code, long pushCount ) const
{
	VarRefLookup lookup = lookupField( pd );

	verifyRefPossible( pd, lookup );

	/* Ensure that the field is referenced. */
	lookup.inObject->referenceField( pd, lookup.objField );

	/* Note that we could have modified children. */
	if ( qual->length() == 0 )
		lookup.objField->refActive = true;

	/* Whenever we take a reference we have to assume writing and that the
	 * tree is dirty. */
	lookup.objField->dirtyTree = true;

	if ( qual->length() > 0 ) {
		if ( lookup.objField->isRhsGet() ) {
			code.append( IN_RHS_REF_FROM_QUAL_REF );
			code.appendHalf( pushCount );

			ObjectField *el = lookup.objField;
			code.append( el->rhsVal.length() );
			for ( Vector<RhsVal>::Iter rg = el->rhsVal; rg.lte(); rg++ ) {
				code.append( rg->prodEl->production->prodNum );
				code.append( rg->prodEl->pos );
			}
		}
		else {
			code.append( IN_REF_FROM_QUAL_REF );
			code.appendHalf( pushCount );
			code.appendHalf( lookup.objField->offset );
			code.appendHalf( lookup.inObject->id );
		}
	}
	else if ( lookup.objField->iterImpl != 0 ) {
		if ( genericLoopVar( lookup.objField ) ) {
			error(loc) << "cannot take a reference to " << lookup.objField->name <<
					", the variable of a loop over a list or a map" << endp;
		}
		code.append( lookup.objField->iterImpl->inRefFromCur );
		code.appendHalf( lookup.objField->offset );
	}
	else if ( lookup.objField->typeRef->type == TypeRef::Ref ) {
		code.append( IN_REF_FROM_REF );
		code.appendHalf( lookup.objField->offset );
	}
	else {
		code.append( IN_REF_FROM_LOCAL );
		code.appendHalf( lookup.objField->offset );
	}

	return lookup.objField;
}

IterImpl *LangVarRef::chooseTriterCall( Compiler *pd,
		UniqueType *searchUT, CallArgVect *args )
{
	IterImpl *iterImpl = 0;

	/* Evaluate the triter args and choose the triter call based on it. */
	if ( args->length() == 1 ) {
		/* Evaluate the expression. A loop over the values of a list or a map
		 * that a context object holds reads it in place. */
		CodeVect unused;
		CallArgVect::Iter pe = *args;
		pd->ctxValueInPlace = forInPlace( pd, searchUT, args );
		pd->throwaway += 1;
		UniqueType *exprUT = (*pe)->expr->evaluate( pd, unused );
		pd->throwaway -= 1;
		pd->ctxValueInPlace = false;

		if ( exprUT->typeId == TYPE_GENERIC && exprUT->generic->typeId == GEN_LIST )
			iterImpl = new IterImpl( IterImpl::ListVal );

		if ( exprUT->typeId == TYPE_GENERIC && exprUT->generic->typeId == GEN_MAP )
			iterImpl = new IterImpl( IterImpl::MapVal );
	}

	if ( iterImpl == 0 )
		iterImpl = new IterImpl( IterImpl::Tree );

	return iterImpl;
}

/* The parameter of func at pos. */
static ObjectField *paramField( Function *func, long pos )
{
	long p = 0;
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++, p++ ) {
		if ( p == pos )
			return param;
	}
	return 0;
}

/* A ref argument takes what a value argument of the parameter's tree type
 * takes, as castAssignment() decides it: a tree of the type, any tree for
 * a ref<any>, an any, which isn't checked, and nil. */
static bool refArgTypeOk( Compiler *pd, UniqueType *paramUT, UniqueType *argUT )
{
	if ( argUT->typeId == TYPE_NIL )
		return true;

	if ( !argUT->treeValue() )
		return false;

	return argUT->langEl == paramUT->langEl ||
			paramUT->langEl == pd->anyLangEl ||
			argUT->langEl == pd->anyLangEl;
}

/* What a site's unwind code is after: a call of a function colm compiles, a
 * parse, or an exit, a yield or a loop's advance. */
enum UnwindSite { UnwindCall, UnwindParse, UnwindOther };

/*
 * The record of the words that the code compiled so far has pushed and not
 * yet consumed (Compiler::exprWords). Each place that leaves a word on the
 * stack while it evaluates more pushes an entry once the word's code is
 * written, and pops it when it writes the instruction that consumes the word.
 * appendUnwindCode() writes the entries' pops into the unwind code of each
 * call, parse and exit, so an unwind through one pops each word as what it
 * is.
 */

/* The kind of word a value of type ut is on the stack. */
static Compiler::ExprWord::Kind exprWordKind( Compiler *pd, UniqueType *ut )
{
	if ( ut->treeValue() || ut->typeId == TYPE_NIL )
		return Compiler::ExprWord::Tree;
	if ( ut->ptr() )
		return Compiler::ExprWord::Struct;
	return Compiler::ExprWord::Value;
}

static Compiler::ExprWord *pushExprWord( Compiler *pd,
		Compiler::ExprWord::Kind kind, long words = 1 )
{
	Compiler::ExprWord *word = new Compiler::ExprWord( kind, words );
	pd->exprWords.append( word );
	return word;
}

/* A word holding a value of type ut. */
static void pushExprValue( Compiler *pd, UniqueType *ut )
{
	pushExprWord( pd, exprWordKind( pd, ut ) );
}

/* The argument area that IN_PREP_ARGS pushes for a call of a function with
 * size words of parameters. Its slots are zeroed, and hold nothing until the
 * arguments are stashed in them. */
static void pushExprArea( Compiler *pd, long size )
{
	Compiler::ExprWord *area = pushExprWord( pd, Compiler::ExprWord::Area, size );
	for ( long i = 0; i < size; i++ )
		area->slots.append( Compiler::ExprWord::Value );
}

/* The innermost argument area, which the arguments being evaluated go into. */
static Compiler::ExprWord *topExprArea( Compiler *pd )
{
	for ( long i = pd->exprWords.length() - 1; i >= 0; i-- ) {
		if ( pd->exprWords[i]->kind == Compiler::ExprWord::Area )
			return pd->exprWords[i];
	}
	assert( false );
	return 0;
}

/* Drops the entries above length, whose words are consumed. */
static void popExprWords( Compiler *pd, long length )
{
	while ( pd->exprWords.length() > length ) {
		long last = pd->exprWords.length() - 1;
		delete pd->exprWords[last];
		pd->exprWords.remove( last );
	}
}

/* The pops of an entry, which release what its words hold. */
static void exprWordPop( Compiler::ExprWord *word, CodeVect &code )
{
	switch ( word->kind ) {
		case Compiler::ExprWord::Tree:
			code.append( IN_POP_TREE );
			break;
		case Compiler::ExprWord::Value:
		case Compiler::ExprWord::Struct:
		case Compiler::ExprWord::Parser:
			/* A struct pops with IN_POP_PTR once structs are counted. */
			code.append( IN_POP_VAL );
			break;
		case Compiler::ExprWord::Plain:
			code.append( IN_POP_N_WORDS );
			code.appendHalf( word->words );
			break;
		case Compiler::ExprWord::Area: {
			/* The arguments of a call not made yet: each slot, from the top,
			 * then the saved call_args, which IN_CLEAR_ARGS puts back. */
			for ( long s = 0; s < word->words; s++ ) {
				Compiler::ExprWord slot( word->slots[s], 1 );
				exprWordPop( &slot, code );
			}
			code.append( IN_CLEAR_ARGS );
			code.appendHalf( 0 );
			break;
		}
	}
}

ObjectField **LangVarRef::evaluateArgs( Compiler *pd, CodeVect &code, 
		VarRefLookup &lookup, CallArgVect *args )
{
	/* Parameter list is given only for user defined methods. Otherwise it
	 * will be null. */
	ParameterList *paramList = lookup.objMethod->paramList;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs != lookup.objMethod->numParams )
		error(loc) << "wrong number of arguments" << endp;

	/* This is for storing the object fields used by references. */
	ObjectField **paramRefs = new ObjectField*[numArgs];
	memset( paramRefs, 0, sizeof(ObjectField*) * numArgs );

	/* Done now if there are no args. */
	if ( args == 0 )
		return paramRefs;

	/* We use this only if there is a paramter list. */
	ParameterList::Iter p;
	long size = 0;
	long tempPops = 0;
	long pos = 0;

	/* The arguments of a function colm compiles go into its argument area.
	 * Others stay on the stack until the call takes them. Either way, the
	 * record has each word that waits. */
	bool area = lookup.objMethod->func != 0 && !lookup.objMethod->func->inHost;

	paramList != 0 && ( p = *paramList );
	for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
		/* Get the expression and the UT for the arg. */
		LangExpr *expression = (*pe)->expr;
		UniqueType *paramUT = lookup.objMethod->paramUTs[pe.pos()];

		/* The argument to a ref parameter of a function or an iterator the
		 * program defines must be a tree of the parameter's type, or nil,
		 * as a value parameter of a tree type takes. The built-in iterators
		 * take a ref<any>, and compileForIter() checks their arguments. */
		bool checkRefArg = paramUT->typeId == TYPE_REF && lookup.objMethod->func != 0;

		if ( paramUT->typeId == TYPE_REF ) {
			/* A child of a switch's variable, named on its own in a case,
			 * is a child of that variable, so a ref to it is a ref to the
			 * child, not a copy. */
			if ( expression->type == LangExpr::TermType &&
					expression->term->type == LangTerm::VarRefType )
				expression->term->varRef->qualifyProdRef( pd );

			if ( expression->canTakeRef( pd ) ) {
				if ( checkRefArg ) {
					CodeVect unused;
					pd->throwaway += 1;
					UniqueType *argUT = expression->evaluate( pd, unused );
					pd->throwaway -= 1;
					if ( !refArgTypeOk( pd, paramUT, argUT ) ) {
						error(loc) << "arg " << pe.pos()+1 <<
								" is of the wrong type" << endp;
					}
				}

				/* Push object loads for reference parameters. */
				LangVarRef *varRef = expression->term->varRef;

				/* A write through the parameter is a write through the
				 * variable the argument starts at. */
				if ( lookup.objMethod->func != 0 ) {
					VarRefLookup argLookup = varRef->lookupField( pd );
					ObjectField *param = paramField( lookup.objMethod->func, pe.pos() );
					pd->refFlow( &argLookup.rootField->refWrite, &param->refWrite );
				}

				ObjectField *refOf = varRef->preEvaluateRef( pd, code );
				paramRefs[pe.pos()] = refOf;

				size += varRef->qual->length() * 2;
				(*pe)->offQualRef = size;
			/**/

				refOf = varRef->evaluateRef( pd, code, 0 ); //(size - (*pe)->offQualRef) );
				paramRefs[pe.pos()] = refOf;

				//size += 2;
			}
			else {
				/* First pass we need to allocate and evaluate temporaries. */
				UniqueType *exprUT = expression->evaluate( pd, code );
				if ( checkRefArg && !refArgTypeOk( pd, paramUT, exprUT ) ) {
					error(loc) << "arg " << pe.pos()+1 <<
							" is of the wrong type" << endp;
				}

				/* An iterator's ref parameter takes no copy: nothing would pop
				 * it after the loop. */
				if ( checkRefArg && lookup.objMethod->iterDef != 0 ) {
					error(loc) << "arg " << pe.pos()+1 << " of iterator " << name <<
							" must be a local, or an attribute or child of one" << endp;
				}

				/* A variable the parameter can't refer to, a global, a
				 * struct's field or a context's variable, is given as a copy,
				 * which the function may only read. checkRefCopies() checks
				 * it once all the code is compiled. A literal or an expression
				 * gets a copy the function can change: nothing sees it. */
				if ( checkRefArg && expression->type == LangExpr::TermType &&
						expression->term->type == LangTerm::VarRefType )
				{
					Compiler::RefCopy copy;
					copy.loc = loc;
					copy.argPos = pe.pos();
					copy.func = lookup.objMethod->func;
					copy.param = paramField( lookup.objMethod->func, pe.pos() );
					copy.loopVar = 0;
					pd->refCopies.append( copy );
				}

				(*pe)->exprUT = exprUT;

				size += 1;
				(*pe)->offTmp = size;
				tempPops += 1;
			/**/
				code.append( IN_REF_FROM_BACK );
				code.appendHalf( 0 ); //size - (*pe)->offTmp );

				//size += 2;
			}

			if ( lookup.objMethod->func ) {
				code.append( IN_STASH_ARG );
				code.appendHalf( pos );
				code.appendHalf( 2 );
			}

			/* What stays on the stack: a qualified variable's qualifier
			 * refs, or the copy. A ref that isn't stashed stays too. */
			if ( expression->canTakeRef( pd ) ) {
				long quals = expression->term->varRef->qual->length() * 2;
				long words = quals + ( lookup.objMethod->func ? 0 : 2 );
				if ( words > 0 )
					pushExprWord( pd, Compiler::ExprWord::Plain, words );
			}
			else {
				pushExprWord( pd, Compiler::ExprWord::Tree );
				if ( !lookup.objMethod->func )
					pushExprWord( pd, Compiler::ExprWord::Plain, 2 );
			}

			if ( area ) {
				Compiler::ExprWord *argArea = topExprArea( pd );
				argArea->slots[pos] = Compiler::ExprWord::Plain;
				argArea->slots[pos+1] = Compiler::ExprWord::Plain;
			}

			pos += 2;
		}
		else {
			UniqueType *exprUT = evaluateSetupValue( pd, code, expression );
			(*pe)->exprUT = exprUT;
			// pd->unwindCode.remove( 0, 1 );

			/* A token action pushes onto its own parser's input, through
			 * input. Other code pushes onto an input that P->gets() gave. */
			if ( ( lookup.objMethod->opcodeWV == IN_INPUT_PUSH_WV ||
					lookup.objMethod->opcodeWV == IN_INPUT_PUSH_IGNORE_WV ) &&
					!isInbuiltObject() )
			{
				checkTreeIntoParse( pd, expression, exprUT, "push",
						", outside a token action", "push its text" );
			}

			if ( !castAssignment( pd, code, paramUT, 0, exprUT ) )
				error(loc) << "arg " << pe.pos()+1 << " is of the wrong type" << endp;

			size += 1;

			if ( area ) {
				code.append( IN_STASH_ARG );
				code.appendHalf( pos );
				code.appendHalf( 1 );
				topExprArea( pd )->slots[pos] = exprWordKind( pd, paramUT );
			}
			else {
				pushExprValue( pd, exprUT );
			}

			pos += 1;
		}

		/* Advance the parameter list iterator if we have it. */
		paramList != 0 && p.increment();
	}

	argSize = tempPops;

	return paramRefs;
}

void LangVarRef::resetActiveRefs( Compiler *pd, VarRefLookup &lookup,
		ObjectField **paramRefs ) const
{
	/* Parameter list is given only for user defined methods. Otherwise it
	 * will be null. */
	for ( long p = 0; p < lookup.objMethod->numParams; p++ ) {
		if ( paramRefs[p] != 0 )
			paramRefs[p]->refActive = false;
	}
}

/* After a call, a parse, which runs parser actions, an exit, a yield, and a
 * loop's advance, where a user iterator's own code runs: the length of the
 * code that unwinds this frame from there, then the code. FN_EXIT and the
 * teardown of a user iterator run it, the return or the parse skips it. It
 * pops the words in the record, top first, then ends the loops of the frame
 * (pd->unwindCode). Where it pops any, IN_EXPR_WORDS first says how many, and
 * where they lie on the stack, for the check of the record. At a call of a
 * function colm compiles, the topmost argument area is the call's own, and
 * so are the words above it: the refs of its ref arguments' qualifiers, and
 * its copies. The callee's frame released its parameters, so the area pops
 * whole. At a parse, the top word is the parser, which FN_EXIT pops itself. */
static void appendUnwindCode( Compiler *pd, CodeVect &code, UnwindSite site )
{
	long top = pd->exprWords.length();
	if ( site == UnwindParse ) {
		assert( top > 0 && pd->exprWords[top-1]->kind == Compiler::ExprWord::Parser );
		top -= 1;
	}

	/* The call's own area. */
	long own = -1;
	if ( site == UnwindCall ) {
		for ( own = top - 1; own >= 0; own-- ) {
			if ( pd->exprWords[own]->kind == Compiler::ExprWord::Area )
				break;
		}
		assert( own >= 0 );
	}

	long count = 0;
	CodeVect pops;
	for ( long i = top - 1; i >= 0; i-- ) {
		Compiler::ExprWord *word = pd->exprWords[i];
		count += word->stackWords();
		if ( i == own ) {
			pops.append( IN_CLEAR_ARGS );
			pops.appendHalf( word->words );
		}
		else {
			exprWordPop( word, pops );
		}
	}

	if ( count == 0 && pd->unwindCode.length() == 0 ) {
		code.appendHalf( 0 );
		return;
	}

	CodeVect check;
	if ( count > 0 ) {
		check.append( IN_EXPR_WORDS );
		check.appendHalf( count );
		if ( pd->unwindLoop != 0 ) {
			check.appendHalf( pd->unwindLoop->iterImpl->inUnwind );
			check.appendHalf( pd->unwindLoop->offset );
		}
		else {
			check.appendHalf( 0 );
			check.appendHalf( 0 );
		}
	}

	code.appendHalf( check.length() + pops.length() + pd->unwindCode.length() + 1 );
	code.append( check );
	code.append( pops );
	code.append( pd->unwindCode );
	code.append( IN_DONE );
}

bool LangVarRef::isFinishCall( VarRefLookup &lookup ) const
{
	return lookup.objMethod->type == ObjectMethod::ParseFinish;
}

void LangVarRef::callOperation( Compiler *pd, CodeVect &code, VarRefLookup &lookup ) const
{
	/* This is for writing if it is a non-const builtin with an undo. One with
	 * the same instruction for both forms records no reverse code: a stream's
	 * close() and auto_trim(), and an input's auto_trim(). Loading its object
	 * for a write that is undone would start a unit of reverse code that
	 * nothing ends, so it loads the object as for a read. */
	bool forWriting = lookup.objMethod->func == 0 && 
			!lookup.objMethod->isConst &&
			lookup.objMethod->opcodeWV != lookup.objMethod->opcodeWC;

	if ( lookup.objMethod->useCallObj ) {
		/* Load the object, if any. */
		loadObj( pd, code, lookup.lastPtrInQual, forWriting );
	}

	/* Check if we need to revert the function. If it operates on a reference
	 * or if it is not local then we need to revert it. */
	bool revert = lookup.lastPtrInQual >= 0 || !isLocalRef() || isInbuiltObject();
	bool unwind = false;
	
	if ( isFinishCall( lookup ) ) {
		/* The parser is under the parse. */
		long words = pd->exprWords.length();
		pushExprWord( pd, Compiler::ExprWord::Parser );

		code.append( IN_SEND_EOF_W );

		LangTerm::parseFrag( pd, code, 0 );
		popExprWords( pd, words );

		code.append( IN_GET_PARSER_MEM_R );
		code.appendHalf( 0 );
	}
	else {
		if ( pd->revertOn && revert )  {
			if ( lookup.objMethod->opcodeWV == IN_CALL_WV ||
					lookup.objMethod->opcodeWC == FN_EXIT )
				unwind = true;

			if ( lookup.objMethod->useFnInstr )
				code.append( IN_FN );
			code.append( lookup.objMethod->opcodeWV );
		}
		else {
			if ( lookup.objMethod->opcodeWC == IN_CALL_WC ||
					lookup.objMethod->opcodeWC == FN_EXIT )
				unwind = true;

			if ( lookup.objMethod->useFnInstr )
				code.append( IN_FN );
			code.append( lookup.objMethod->opcodeWC );
		}
	}
	
	if ( lookup.objMethod->useFuncId )
		code.appendHalf( lookup.objMethod->funcId );

	if ( lookup.objMethod->useGenericId )
		code.appendHalf( lookup.objMethod->generic->id );

	if ( unwind ) {
		appendUnwindCode( pd, code, lookup.objMethod->func != 0 ?
				UnwindCall : UnwindOther );
	}
}

void LangVarRef::popRefQuals( Compiler *pd, CodeVect &code, 
		VarRefLookup &lookup, CallArgVect *args, bool temps ) const
{
	/* Each ref argument left words on the stack: the refs of its
	 * qualification, or the value it was given as a copy. The last
	 * argument's are on top, so pop from the last argument to the first. */
	if ( args != 0 ) {
		for ( long a = args->length() - 1; a >= 0; a-- ) {
			/* Get the expression and the UT for the arg. */
			LangExpr *expression = args->data[a]->expr;
			UniqueType *paramUT = lookup.objMethod->paramUTs[a];

			if ( paramUT->typeId == TYPE_REF ) {
				if ( expression->canTakeRef( pd ) ) {
					LangVarRef *varRef = expression->term->varRef;
					long popCount = varRef->qual->length() * 2;
					if ( popCount > 0 ) {
						code.append( IN_POP_N_WORDS );
						code.appendHalf( (short)popCount );
					}
				}
				else if ( temps )
					code.append( IN_POP_TREE );
			}
		}
	}
}


/* A call of func is made while the running context is lent, or, for a
 * function, while a parameter is lent. checkLentCalls() refuses
 * it once all the code is compiled if func names the context, or can yield
 * to a loop's body, which could use what is lent. */
static void noteLentCall( Compiler *pd, Function *func, const InputLoc &loc )
{
	if ( func == 0 || !pd->revertOn || pd->curBlock == 0 )
		return;

	for ( int i = 0; i < pd->owns.length(); i++ ) {
		Compiler::Own &own = pd->owns[i];
		bool lent = own.kind == Compiler::Own::LentToParse ||
				own.kind == Compiler::Own::LentToParser ||
				own.kind == Compiler::Own::LentToLoop;
		bool running = own.var == 0 && lent;
		bool param = own.var != 0 && own.var->isParam() && lent && !func->isUserIter;
		if ( running || param ) {
			CodeBlock::LentCall call;
			call.func = func;
			call.loc = loc;
			call.lentLoc = own.loc;
			call.running = running;
			call.what = running ? String( "the context object" ) : own.var->name;
			pd->curBlock->lentCalls.append( call );
		}
	}
}

/* A function lends a parameter as if nothing else reached its object, so
 * two of the arguments of a context type can't reach one object: C and C,
 * or C and C->Sub. An argument that the running context reaches can't go to
 * a function that names the context, which checkLentCalls() finds. The
 * arguments of a context type that a variable reaches go in ctxArgs, for
 * checkContextArgsKept(). An argument past the parameters is left to
 * evaluateArgs(), which says the count is wrong. */
static void checkContextArgs( Compiler *pd, Function *func, UniqueType **paramUTs,
		long numParams, CallArgVect *args, Vector<LangVarRef*> &ctxArgs )
{
	if ( func == 0 || args == 0 )
		return;

	Vector<ObjectField*> roots;
	for ( int a = 0; a < args->length() && a < numParams; a++ ) {
		UniqueType *paramUT = paramUTs[a];
		LangExpr *argExpr = args->data[a]->expr;
		ObjectField *root;
		if ( paramUT->typeId != TYPE_STRUCT || !paramUT->structEl->structDef->isContext ||
				argExpr->type != LangExpr::TermType ||
				argExpr->term->type != LangTerm::VarRefType ||
				!argExpr->term->varRef->ownRoot( pd, root ) )
			continue;

		LangVarRef *argRef = argExpr->term->varRef;
		ctxArgs.append( argRef );
		for ( int r = 0; r < roots.length(); r++ ) {
			if ( roots[r] == root ) {
				error(argRef->loc) << "cannot give " << func->name <<
						" two arguments that " << ( root == 0 ?
						String( "the context object" ) : root->name ) <<
						" reaches: it could lend one and use the other" << endp;
			}
		}
		roots.append( root );

		if ( root == 0 && pd->revertOn && pd->curBlock != 0 ) {
			CodeBlock::ContextArg arg;
			arg.func = func;
			arg.loc = argRef->loc;
			arg.arg = argRef->qual->length() > 0 ? argRef->qual->data[0].data :
					argRef->name;
			pd->curBlock->contextArgs.append( arg );
		}
	}
}

/* An argument after one of a context type can take its object away, as
 * new parser<T>( C ) does, so that the function gets the object twice: as
 * the parameter, and in the parser. Once the arguments are made, the object
 * each of ctxArgs gave must still be where it was. */
static void checkContextArgsKept( Compiler *pd, Function *func,
		const Vector<LangVarRef*> &ctxArgs )
{
	for ( int a = 0; a < ctxArgs.length(); a++ ) {
		ObjectField *root;
		ctxArgs[a]->ownRoot( pd, root );
		Compiler::Own *own = pd->findOwn( root );
		if ( own == 0 || root == 0 )
			continue;

		if ( own->kind == Compiler::Own::Moved ||
				own->kind == Compiler::Own::MovedWithParser )
		{
			std::ostream &out = error(ctxArgs[a]->loc) << "cannot give " <<
					func->name << " " << root->name << ": another argument " <<
					( own->kind == Compiler::Own::Moved ? "moves it at " :
					"takes it with the parser made at " );
			ownPlace( out, own->loc );
			out << endp;
		}
	}
}

UniqueType *LangVarRef::evaluateCall( Compiler *pd, CodeVect &code, CallArgVect *args ) 
{
	/* Evaluate the object. */
	VarRefLookup lookup = lookupMethod( pd );

	/* An iterator runs only as the iterator of a for loop. */
	if ( lookup.objMethod->iterDef != 0 )
		error(loc) << "iterator " << name << " called outside a for loop" << endp;

	Function *func = lookup.objMethod->func;

	/* A function can yield for the iterator that calls it, so what it
	 * yields goes where the caller's yields go. */
	if ( func != 0 && pd->curFunction != 0 )
		pd->refFlow( &func->yieldWrite, &pd->curFunction->yieldWrite );

	/* What it is called on is used before its arguments are, as the error
	 * says, though callOperation() loads it after them. */
	if ( qual->length() > 0 )
		checkOwnedUse( pd );

	/* A call that changes what it is called on. A parser's finish parses. */
	bool change = func == 0 && ( !lookup.objMethod->isConst ||
			lookup.objMethod->type == ObjectMethod::ParseFinish );

	/* Prepare the contiguous call args space. */
	int asLoc;
	long words = pd->exprWords.length();
	if ( func != 0 && !func->inHost ) {
		code.append( IN_PREP_ARGS );
		asLoc = code.length();
		code.appendHalf( 0 );
		pushExprArea( pd, func->paramListSize );
	}

	/* The parser variable a finish() or a gets() is called on, if it is lent
	 * an object. */
	ObjectField *parserVar = 0;
	bool gets = lookup.objMethod->opcodeWC == IN_GET_PARSER_STREAM;
	if ( qual->length() == 1 && isLocalRef() &&
			( lookup.objMethod->type == ObjectMethod::ParseFinish || gets ) )
	{
		ObjectField *var = scope->findField( qual->data[0].data );
		if ( var != 0 && var->typeRef->uniqueType->parser() )
			parserVar = var;
	}

	if ( parserVar != 0 ) {
		Compiler::Own *fin = pd->findOwn( parserVar );
		if ( fin != 0 && fin->kind == Compiler::Own::Finished ) {
			std::ostream &out = error(loc) << ( gets ?
					"cannot get the input of " : "cannot finish " ) <<
					parserVar->name << ": it was finished at ";
			ownPlace( out, fin->loc );
			out << endp;
		}
		if ( gets && pd->lentToParser( parserVar ) ) {
			error(loc) << "cannot get the input of " << parserVar->name <<
					": it is lent a context object until " << parserVar->name <<
					"->finish()" << endp;
		}
	}

	noteLentCall( pd, func, loc );
	Vector<LangVarRef*> ctxArgs;
	checkContextArgs( pd, func, lookup.objMethod->paramUTs,
			lookup.objMethod->numParams, args, ctxArgs );

	/* Evaluate and push the arguments. A change through a context reference
	 * puts them into the object. */
	if ( change )
		setCtxStore( pd, this, lookup );
	ObjectField **paramRefs = evaluateArgs( pd, code, lookup, args );
	pd->ctxStore = false;
	checkContextArgsKept( pd, func, ctxArgs );

	/* The instruction of a built-in or a host function takes its arguments.
	 * What popRefQuals() pops after it lies under no call. */
	if ( func == 0 || func->inHost )
		popExprWords( pd, words );

	checkContextValue( pd, lookup, lookup.uniqueType, false );

	/* A call on what a global holds. A change given a reference value puts
	 * it in, as push_tail and insert do. */
	if ( qual->length() > 0 ) {
		if ( change )
			noteGlobalUse( pd, GlobalUse::Change, String(), argHeldRef( pd, args ) );
		if ( pd->revertOn && heldRef( pd, lookup.uniqueType ) )
			noteGlobalUse( pd, GlobalUse::Take, "what " + name + " gives" );
	}

	if ( func != 0 || lookup.objMethod->type == ObjectMethod::ParseFinish )
		pd->countOtherCode();

	/* A call that changes what it is called on is the program's own code,
	 * for a parse's items, as a call of a function is. */
	if ( change )
		pd->otherCodeRuns += 1;

	if ( func != 0 && pd->revertOn && pd->curBlock != 0 )
		pd->curBlock->calls.append( func );

	/* Write the call opcode. */
	callOperation( pd, code, lookup );

	/* A finished parser gives back what it was lent. */
	if ( parserVar != 0 && lookup.objMethod->type == ObjectMethod::ParseFinish &&
			pd->lentToParser( parserVar ) && pd->throwaway == 0 )
	{
		pd->releaseParser( parserVar );
		pd->addOwn( parserVar, Compiler::Own::Finished, loc );
	}

	popRefQuals( pd, code, lookup, args, true );

	resetActiveRefs( pd, lookup, paramRefs);
	delete[] paramRefs;

	if ( func != 0 && !func->inHost ) {
		code.append( IN_CLEAR_ARGS );
		code.appendHalf( func->paramListSize );
		code.setHalf( asLoc, func->paramListSize );
		popExprWords( pd, words );
	}

	if ( func != 0 && !func->inHost )
		code.append( IN_LOAD_RETVAL );

	/* Return the type to the expression. A weak result is read as its
	 * target. */
	return lookup.uniqueType->readType();
}

/* Can match on a tree or a ref. A tree always comes back. */
UniqueType *LangTerm::evaluateMatch( Compiler *pd, CodeVect &code ) const
{
	/* Add the vars bound by the pattern into the local scope. */
	for ( PatternItemList::Iter item = *pattern->list; item.lte(); item++ ) {
		if ( item->varRef != 0 )
			item->bindId = pattern->nextBindId++;
	}

	UniqueType *ut = varRef->evaluate( pd, code );
	if ( !ut->treeValue() ) {
		error(varRef->loc) << "expected match against a tree/ref type" << endp;
	}

	/* Store the language element type in the pattern. This is needed by
	 * the pattern parser. */
	pattern->langEl = ut->langEl;

	code.append( IN_MATCH );
	code.appendHalf( pattern->patRepId );

	for ( PatternItemList::Iter item = pattern->list->last(); item.gtb(); item-- ) {
		if ( item->varRef != 0 ) {
			/* Compute the unique type. */
			UniqueType *exprType = pd->findUniqueType( TYPE_TREE, item->prodEl->langEl );

			/* Get the type of the variable being assigned to. */
			VarRefLookup lookup = item->varRef->lookupField( pd );

			item->varRef->loadObj( pd, code, lookup.lastPtrInQual, false );
			item->varRef->setField( pd, code, lookup.inObject,
					lookup.objField, exprType, false );
		}
	}

	/* The process of matching turns refs into trees. */
	if ( ut->typeId == TYPE_REF )
		ut = pd->findUniqueType( TYPE_TREE, ut->langEl );

	return ut;
}

UniqueType *LangTerm::evaluateProdCompare( Compiler *pd, CodeVect &code ) const
{
	UniqueType *ut = varRef->evaluate( pd, code );
	if ( !ut->treeValue() ) {
		error(varRef->loc) << "expected match against a tree/ref type" << endp;
	}
	code.append( IN_PROD_NUM );

	/* look up the production name. */
	Production *prod = pd->findProductionByLabel( ut->langEl, this->prod );

	if ( prod == 0 ) {
		error( this->loc) << "could not find "
				"production label: " << this->prod << endp;
	}

	unsigned int n = prod->prodNum;
	code.append( IN_LOAD_INT );
	code.appendWord( n );

	code.append( IN_TST_EQL_VAL );

	if ( expr != 0 ) {
		code.append( IN_DUP_VAL );

		/* Test: jump past the match if the production test failed. We don't have
		 * the distance yet. */
		long jumpFalse = code.length();
		code.append( IN_JMP_FALSE_VAL );
		code.appendHalf( 0 );

		code.append( IN_POP_VAL );

		expr->evaluate( pd, code );

		/* Set the jump false distance. */
		long falseDist = code.length() - jumpFalse - 3;
		code.setHalf( jumpFalse+1, falseDist );

		return ut;
	}

	return pd->uniqueTypeInt;
}

void LangTerm::evaluateCapture( Compiler *pd, CodeVect &code, UniqueType *valUt ) const
{
	if ( varRef != 0 ) {
		/* Get the type of the variable being assigned to. */
		VarRefLookup lookup = varRef->lookupField( pd );

		/* Need a copy of the tree. */
		code.append( lookup.uniqueType->tree() ? IN_DUP_TREE : IN_DUP_VAL );

		varRef->loadObj( pd, code, lookup.lastPtrInQual, false );
		varRef->setField( pd, code, lookup.inObject, lookup.objField, valUt, false );
	}
}

/* The actions of a type defined in a context use the context's variables, so
 * a parse of it, or a parser made with new, must be given an object of the
 * context. */
static void checkContextGiven( const InputLoc &loc, LangEl *langEl, bool given )
{
	if ( langEl->contextIn != 0 && !given ) {
		error( loc ) << "parsing " << langEl->declName << " needs a context of type " <<
				langEl->contextIn->name << endp;
	}
}

/* A struct's name qualified by the namespaces it is declared in, as a type's
 * xmlTag is. The root namespace, which has no parent, adds nothing. */
static String qualifiedName( StructEl *structEl )
{
	String name = structEl->name;
	for ( Namespace *nspace = structEl->nspace; nspace != 0 &&
			nspace->parentNamespace != 0; nspace = nspace->parentNamespace )
		name = nspace->name + "::" + name;
	return name;
}

/* A parser of a type defined in a context runs its actions on an object of
 * that context, so it must be given one: another struct isn't laid out like
 * the context. */
static void checkParserContext( const FieldInit *arg, UniqueType *argUT,
		LangEl *langEl )
{
	StructDef *context = langEl->contextIn;
	if ( argUT->structEl == context->structEl )
		return;

	/* An expression that is a single term has its location in the term. */
	const InputLoc &loc = arg->expr->type == LangExpr::TermType ?
			arg->expr->term->loc : arg->expr->loc;
	error( loc ) << "the context of " << langEl->xmlTag << " must be of type " <<
			qualifiedName( context->structEl ) << ", not " <<
			qualifiedName( argUT->structEl ) << endp;
}

/* The context a def or a token is defined in, or 0. A literal's is the
 * context of the lex block that defines it. */
static StructDef *elContext( LangEl *lel )
{
	if ( lel->isLiteral && lel->tokenDef != 0 )
		return lel->tokenDef->contextIn;
	return lel->contextIn;
}

/* The first def or token, in production order, that the grammar of lel uses,
 * directly or through other defs, and that is defined in a context other than
 * own. The defs of own and the defs outside any context are looked through. */
static LangEl *otherContextEl( LangEl *lel, StructDef *own, BstSet<LangEl*> &seen )
{
	if ( !seen.insert( lel ) )
		return 0;

	for ( LelProdList::Iter prod = lel->prodList; prod.lte(); prod++ ) {
		for ( ProdElList::Iter pe = *prod->prodElList; pe.lte(); pe++ ) {
			LangEl *el = pe->langEl;
			if ( el == 0 )
				continue;

			StructDef *context = elContext( el );
			if ( context != 0 && context != own )
				return el;

			if ( el->type == LangEl::NonTerm ) {
				LangEl *found = otherContextEl( el, own, seen );
				if ( found != 0 )
					return found;
			}
		}
	}
	return 0;
}

/* A parse runs the actions of the defs and tokens its type's grammar uses, and
 * an action written in a context uses the context the parse is given. A parse
 * can be given only the context of its own type, so a type whose grammar uses
 * the defs or tokens of another context can't be parsed. */
static void checkGrammarContext( const InputLoc &loc, LangEl *root )
{
	BstSet<LangEl*> seen;
	LangEl *el = otherContextEl( root, root->contextIn, seen );
	if ( el != 0 ) {
		String context = qualifiedName( elContext( el )->structEl );
		error( loc ) << root->declName << " can't be parsed: its grammar uses " <<
				el->declName << ", of context " << context << ", and " <<
				root->declName << " isn't in " << context << endp;
	}
}

/* A context object leaves the place varRef names, which root is the root of.
 * A local's moves. Nothing moves out of a parameter, which the caller lends,
 * of this, or of a field, which is given another object in its place. */
static void moveOut( Compiler *pd, const LangVarRef *varRef, ObjectField *root )
{
	if ( varRef->qual->length() > 0 ) {
		error(varRef->loc) << "cannot move " << varRef->name << " out of " <<
				varRef->qual->data[varRef->qual->length()-1].data <<
				": give the field another object in its place" << endp;
	}
	if ( root == 0 && varRef->isInbuiltObject() )
		error(varRef->loc) << "cannot move this: the parse lends it for the action" << endp;
	if ( root == 0 ) {
		error(varRef->loc) << "cannot move " << varRef->name <<
				" out of the context object: give the field another object "
				"in its place" << endp;
	}
	if ( root->isParam() ) {
		error(varRef->loc) << "cannot move " << varRef->name <<
				": the caller lends it for the call" << endp;
	}
}

/* A value of a context type that a local, a parameter, this or a field holds,
 * put into another place or returned, moves. */
static void noteMove( Compiler *pd, LangExpr *expr )
{
	if ( expr == 0 || expr->type != LangExpr::TermType ||
			expr->term->type != LangTerm::VarRefType )
		return;

	LangVarRef *varRef = expr->term->varRef;
	UniqueType *ut = varRef->lookup( pd );
	if ( ut->typeId != TYPE_STRUCT || !ut->structEl->structDef->isContext )
		return;

	ObjectField *root;
	if ( !varRef->ownRoot( pd, root ) )
		return;

	moveOut( pd, varRef, root );
	if ( pd->throwaway == 0 )
		pd->addOwn( root, Compiler::Own::Moved, varRef->loc );
}

UniqueType *LangTerm::evaluateNew( Compiler *pd, CodeVect &code ) const
{
	/* What is being newstructed. */
	UniqueType *newUT = typeRef->uniqueType;

	if ( newUT->typeId != TYPE_STRUCT && newUT->typeId != TYPE_GENERIC )
		error(loc) << "can only new a struct or generic" << endp;

	/* An input works through an impl that only a parser makes. */
	if ( newUT == pd->uniqueTypeInput )
		error(loc) << "cannot new an _input: a parser makes its input" << endp;

	/* A parser gets its input from sends, never from new. One of a type in a
	 * context is given the context, and nothing else. */
	bool context = false;
	if ( newUT->parser() ) {
		LangEl *langEl = newUT->generic->elUt->langEl;
		checkGrammarContext( loc, langEl );

		/* The program sends to it, and runs between the sends. */
		pd->restartTypes.insert( langEl );

		if ( langEl->contextIn != 0 ) {
			if ( fieldInitArgs != 0 && fieldInitArgs->length() > 1 ) {
				error(loc) << "new parser<" << langEl->declName <<
						"> takes one argument, its context" << endp;
			}
			context = true;
		}
		else if ( fieldInitArgs != 0 && fieldInitArgs->length() > 0 ) {
			error(loc) << "new parser<" << langEl->declName <<
					"> takes no arguments" << endp;
		}
	}

	if ( newUT->typeId == TYPE_GENERIC ) {
		code.append( IN_CONS_GENERIC );
		code.appendHalf( newUT->generic->id );
		code.appendHalf( 0 ); // stopId
	}
	else if ( newUT->typeId == TYPE_STRUCT && newUT->structEl == pd->streamSel ) {
		code.append( IN_NEW_STREAM );
	}
	else {
		code.append( IN_NEW_STRUCT );
		code.appendHalf( newUT->structEl->id );
	}

	/* The parser variable the new parser goes into, if any. */
	ObjectField *parserVar = pd->newParserVar;
	pd->newParserVar = 0;

	/*
	 * First load the context into the parser, which waits under it.
	 */
	if ( context ) {
		long words = pd->exprWords.length();
		pushExprWord( pd, Compiler::ExprWord::Parser );

		bool contextGiven = false;
		for ( int i = 0; fieldInitArgs != 0 && i < fieldInitArgs->length(); i++ ) {
			/* Eval what we are initializing with. */
			LangExpr *argExpr = fieldInitArgs->data[i]->expr;
			UniqueType *argUT = argExpr->evaluate( pd, code );

			if ( argUT->typeId == TYPE_STRUCT ) {
				checkParserContext( fieldInitArgs->data[i], argUT,
						newUT->generic->elUt->langEl );
				code.append( IN_SET_PARSER_CONTEXT );
				contextGiven = true;

				/* The root of the object is lent to the parser variable
				 * the parser goes into, once the assignment has ended what
				 * the variable had. Anywhere else, the object goes with
				 * the parser for good. */
				LangVarRef *argRef = argExpr->type == LangExpr::TermType &&
						argExpr->term->type == LangTerm::VarRefType ?
						argExpr->term->varRef : 0;
				ObjectField *root;
				if ( argRef != 0 && argRef->ownRoot( pd, root ) ) {
					if ( parserVar != 0 ) {
						pd->newParserLent = true;
						pd->newParserRoot = root;
						pd->newParserLoc = loc;
						pd->newParserField = argRef->qual->length() > 0;
					}
					else {
						moveOut( pd, argRef, root );
						if ( pd->throwaway == 0 )
							pd->addOwn( root, Compiler::Own::MovedWithParser, loc );
					}
				}
			}
			else {
				error(loc) << "new parser<" << newUT->generic->elUt->langEl->declName <<
						"> takes one argument, its context" << endp;
			}
		}

		checkContextGiven( loc, newUT->generic->elUt->langEl, contextGiven );
		popExprWords( pd, words );
	}

	evaluateCapture( pd, code, newUT );

	return newUT;
}

/* A cast gives a copy of a tree under another tree type. Inside colm a str is
 * a tree too, so a str can be cast, but nothing can be cast to one: the copy
 * would keep the tree's ignores and children, which a str never frees, and a
 * def has no text to give it. */
UniqueType *LangTerm::evaluateCast( Compiler *pd, CodeVect &code ) const
{
	UniqueType *ut = typeRef->uniqueType;
	if ( ut->typeId != TYPE_TREE || ut->langEl == pd->strLangEl )
		error(loc) << "can only cast to a tree type" << endp;

	/* An il holds ignored text in its children and has no text of its own.
	 * A cast would give it the text of X, which is never printed or freed. */
	if ( ut->langEl == pd->ignoreLangEl )
		error(loc) << "cannot cast to il" << endp;

	UniqueType *exprUT = expr->evaluate( pd, code );
	if ( !exprUT->treeValue() )
		error(loc) << "can only cast a tree" << endp;

	/* A cast keeps to its kind. A token's text is its own, and a def's is in
	 * its children, as an il's is. A cast across them would give a token with
	 * no text, or a def whose text never prints. A cast of an any is checked
	 * when it runs, and a cast to any isn't checked. */
	LangEl *from = exprUT->langEl, *to = ut->langEl;
	if ( from != 0 && from != pd->anyLangEl && to != pd->anyLangEl ) {
		bool fromToken = from->type == LangEl::Term && from != pd->ignoreLangEl;
		bool toToken = to->type == LangEl::Term;
		if ( toToken && from == pd->ignoreLangEl )
			error(loc) << "cannot cast il to a token type" << endp;
		if ( toToken && !fromToken )
			error(loc) << "cannot cast a def to a token type" << endp;
		if ( !toToken && fromToken )
			error(loc) << "cannot cast a token or str to a def type" << endp;
	}

	code.append( IN_TREE_CAST );
	code.appendHalf( ut->langEl->id );
	return ut;
}

void LangTerm::assignFieldArgs( Compiler *pd, CodeVect &code, UniqueType *replUT ) const
{
	/* Now assign the field initializations. Note that we need to do this in
	 * reverse because the last expression evaluated is at the top of the
	 * stack. */
	if ( fieldInitArgs != 0 && fieldInitArgs->length() > 0 ) {
		ObjectDef *objDef = replUT->objectDef();
		/* Note the reverse traversal. */
		for ( FieldInitVect::Iter pi = fieldInitArgs->last(); pi.gtb(); pi-- ) {
			FieldInit *fieldInit = *pi;
			ObjectField *field = objDef->findFieldNum( pi.pos() );
			if ( field == 0 ) {
				error(fieldInit->loc) << "failed to find init pos " << 
					pi.pos() << " in object" << endp;
			}

			/* Lookup the type of the field and compare it to the type of the
			 * expression. */
			UniqueType *fieldUT = field->typeRef->uniqueType;
			if ( !castAssignment( pd, code, fieldUT, 0, fieldInit->exprUT ) )
				error(fieldInit->loc) << "type mismatch in initialization" << endp;

			/* The set field instruction must leave the object on the top of
			 * the stack. */
			code.append( IN_SET_FIELD_TREE_LEAVE_WC );
			code.appendHalf( field->offset );
		}
	}
}

UniqueType *LangTerm::evaluateConstruct( Compiler *pd, CodeVect &code ) const
{
	/* Evaluate the initialization expressions. Each waits under the rest,
	 * and the items, until it is set. */
	long words = pd->exprWords.length();
	if ( fieldInitArgs != 0 && fieldInitArgs->length() > 0 ) {
		for ( FieldInitVect::Iter pi = *fieldInitArgs; pi.lte(); pi++ ) {
			FieldInit *fieldInit = *pi;
			fieldInit->exprUT = fieldInit->expr->evaluate( pd, code );

			/* A value goes into an attribute wrapped, as an assignment
			 * stores it. */
			if ( fieldInit->exprUT->ptr() || fieldInit->exprUT->scalar() ) {
				code.append( IN_VAL_TO_POINTER );
				pushExprWord( pd, Compiler::ExprWord::Tree );
			}
			else {
				pushExprValue( pd, fieldInit->exprUT );
			}
		}
	}
	long fieldWords = pd->exprWords.length();

	/* Assign bind ids to the variables in the replacement. */
	for ( ConsItemList::Iter item = *constructor->list; item.lte(); item++ ) {
		if ( item->expr != 0 )
			item->bindId = constructor->nextBindId++;
	}

	/* Evaluate variable references. */
	for ( ConsItemList::Iter item = constructor->list->last(); item.gtb(); item-- ) {
		if ( item->type == ConsItem::ExprType ) {
			UniqueType *ut = item->expr->evaluate( pd, code );
		
			if ( ut->typeId != TYPE_TREE ) {
				error(constructor->loc) << "variables used in "
						"replacements must be trees" << endp;
			}

			if ( !isStr( ut ) ) {
				if ( item->trim == ConsItem::TrimYes ) 
					code.append( IN_TREE_TRIM );
			}

			item->langEl = ut->langEl;

			/* It waits under the items before it, which come after. */
			pushExprWord( pd, Compiler::ExprWord::Tree );
		}
	}

	/* Construct the tree using the tree information stored in the compiled
	 * code. */
	code.append( IN_CONSTRUCT );
	code.appendHalf( constructor->patRepId );
	popExprWords( pd, fieldWords );

	/* Lookup the type of the replacement and store it in the replacement
	 * object so that replacement parsing has a target. */
	UniqueType *replUT = typeRef->uniqueType;
	if ( replUT->typeId != TYPE_TREE )
		error(loc) << "don't know how to construct this type" << endp;
	
	constructor->langEl = replUT->langEl;
	assignFieldArgs( pd, code, replUT );
	popExprWords( pd, words );

	evaluateCapture( pd, code, replUT );

	return replUT;
}

void LangTerm::parseFrag( Compiler *pd, CodeVect &code, int stopId )
{
	code.append( IN_PARSE_FRAG_W );
	appendUnwindCode( pd, code, UnwindParse );
}

UniqueType *LangTerm::evaluateReadReduce( Compiler *pd, CodeVect &code ) const
{
	UniqueType *parserUT = typeRef->uniqueType;
	UniqueType *targetUT = parserUT->generic->elUt;

	/* Should be one arg and it should be a stream. */

	/* Assign bind ids to the variables in the replacement. */
	for ( ConsItemList::Iter item = *parserText->list; item.lte(); item++ ) {
		switch ( item->type ) {
		case ConsItem::LiteralType: {
			break;
		}
		case ConsItem::InputText: {
			break;
		}
		case ConsItem::ExprType: {
			item->expr->evaluate( pd, code );
			break;
		}}
	}	

	code.append( IN_READ_REDUCE );
	code.appendHalf( parserUT->generic->id );
	code.appendHalf( parserText->reducerId );

	return targetUT;
}

/* The items of a print, a send, a parse and a string go out as text, or as
 * trees. A struct, list, map, parser or stream isn't a tree, and the runtime
 * would read it as one. A parser reads a stream sent to it in full. */
static void checkSendItem( Compiler *pd, const ConsItem *item, UniqueType *ut,
		bool streamOk, const char *message )
{
	switch ( ut->typeId ) {
		case TYPE_TREE:
		case TYPE_REF:
		case TYPE_NIL:
		case TYPE_INT:
		case TYPE_BOOL:
			return;
		default:
			break;
	}

	if ( streamOk && ut == pd->uniqueTypeStream )
		return;

	/* An expression that is a single term has its location in the term. */
	const InputLoc &loc = item->expr->type == LangExpr::TermType ?
			item->expr->term->loc : item->expr->loc;
	error( loc ) << message << endp;
}

UniqueType *LangTerm::evaluateParse( Compiler *pd, CodeVect &code,
		bool tree, bool stop ) const
{
	pd->countOtherCode();

	UniqueType *parserUT = typeRef->uniqueType;
	UniqueType *targetUT = parserUT->generic->elUt;

	/* Backing up over an action backs up a parse it started. */
	if ( pd->revertOn && pd->curBlock != 0 )
		pd->curBlock->parses.append( targetUT->langEl );

	if ( parserText->reduce && parserText->read ) {
		return evaluateReadReduce( pd, code );
	}

	/* If this is a parse stop then we need to verify that the type is
	 * compatible with parse stop. */
	if ( stop )
		targetUT->langEl->parseStop = true;
	int stopId = stop ? targetUT->langEl->id : 0;

	checkGrammarContext( loc, targetUT->langEl );

	bool context = false;
	if ( fieldInitArgs != 0 ) {
		if ( fieldInitArgs == 0 || ( fieldInitArgs->length() != 1 && fieldInitArgs->length() != 2 ) )
			error(loc) << "parse command requires just context and input" << endp;
		context = true;
	}

	/* Construct the parser. */

	if ( parserText->reduce ) {
		code.append( IN_CONS_REDUCER );
		code.appendHalf( parserUT->generic->id );
		code.appendHalf( parserText->reducerId );
	}
	else {
		code.append( IN_CONS_GENERIC );
		code.appendHalf( parserUT->generic->id );
		code.appendHalf( stopId );
	}

	/* The parser waits under its context, its items, and the parses. */
	long words = pd->exprWords.length();
	pushExprWord( pd, Compiler::ExprWord::Parser );

	/*
	 * First load the context into the parser.
	 */
	bool contextGiven = false;
	bool lent = false;
	ObjectField *lentRoot = 0;
	if ( context ) {
		for ( int i = 0; i < fieldInitArgs->length(); i++ ) {
			/* Eval what we are initializing with. */
			UniqueType *argUT = fieldInitArgs->data[i]->expr->evaluate( pd, code );

			if ( argUT == pd->uniqueTypeInput ) {
				code.append( IN_SET_PARSER_INPUT );
			}
			else if ( argUT->typeId == TYPE_STRUCT && targetUT->langEl->contextIn != 0 ) {
				checkParserContext( fieldInitArgs->data[i], argUT,
						targetUT->langEl );

				/* One context, as new parser takes: a second would replace
				 * the first, which would stay held. */
				if ( contextGiven ) {
					const FieldInit *arg = fieldInitArgs->data[i];
					const InputLoc &argLoc = arg->expr->type == LangExpr::TermType ?
							arg->expr->term->loc : arg->expr->loc;
					error(argLoc) << "parse command requires just context and input" << endp;
				}

				code.append( IN_SET_PARSER_CONTEXT );
				contextGiven = true;

				/* The parse is lent its context object for its statement. */
				LangExpr *argExpr = fieldInitArgs->data[i]->expr;
				if ( argExpr->type == LangExpr::TermType &&
						argExpr->term->type == LangTerm::VarRefType &&
						argExpr->term->varRef->ownRoot( pd, lentRoot ) )
				{
					lent = true;
					pd->addOwn( lentRoot, Compiler::Own::LentToParse, loc );
				}
			}
			else {
				error(loc) << "cannot initialize parser with this type, context or input only" << endp;
			}
		}
	}

	checkContextGiven( loc, targetUT->langEl, contextGiven );

	/*****************************/

	if ( parserText->list->length() == 0 ) {
		code.append( IN_SEND_NOTHING );

		/* Parse instruction, dependent on whether or not we are producing
		 * revert or commit code. */
		parseFrag( pd, code, stopId );
	}
	else {
		for ( ConsItemList::Iter item = *parserText->list; item.lte(); item++ ) {
			bool isStream = false;
			uchar trim = TRIM_DEFAULT;

			switch ( item->type ) {
			case ConsItem::LiteralType: {
				String result;
				bool unusedCI;
				prepareLitString( result, unusedCI, 
						item->prodEl->typeRef->pdaLiteral->data,
						item->prodEl->typeRef->pdaLiteral->loc );

				/* Make sure we have this string. */
				StringMapEl *mapEl = 0;
				if ( pd->literalStrings.insert( result, &mapEl ) )
					mapEl->value = pd->literalStrings.length()-1;

				code.append( IN_LOAD_STR );
				code.appendWord( mapEl->value );
				break;
			}
			case ConsItem::InputText: {
				/* Make sure we have this string. */
				StringMapEl *mapEl = 0;
				if ( pd->literalStrings.insert( item->data, &mapEl ) )
					mapEl->value = pd->literalStrings.length()-1;

				code.append( IN_LOAD_STR );
				code.appendWord( mapEl->value );
				break;
			}
			case ConsItem::ExprType: {
				/* Other code run by an item after the first runs while the
				 * parser can back up over what was sent before it. */
				long runs = pd->otherCodeRuns;
				UniqueType *ut = item->expr->evaluate( pd, code );
				if ( item.ptr != parserText->list->head && pd->otherCodeRuns != runs )
					pd->restartTypes.insert( targetUT->langEl );

				if ( ut->typeId == TYPE_VOID ) {
					/* Clear it away if return type is void. */
					code.append( IN_POP_VAL );
					continue;
				}

				checkSendItem( pd, item, ut, true, "only a tree, str, int, bool "
						"or stream can be sent to a parser" );
				if ( tree ) {
					checkTreeIntoParse( pd, item->expr, ut, "parse_tree", "",
							"parse its text" );
				}

				if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
					code.append( IN_INT_TO_STR );

				if ( ut == pd->uniqueTypeStream )
					isStream = true;

				if ( item->trim == ConsItem::TrimYes )
					trim = TRIM_YES;
				else if ( item->trim == ConsItem::TrimNo )
					trim = TRIM_NO;

				break;
			}}

			if ( isStream )
				code.append( IN_SEND_STREAM_W );
			else if ( tree ) {
				code.append( IN_SEND_TREE_W );
				code.append( trim );
			}
			else {
				code.append( IN_SEND_TEXT_W );
				code.append( trim );
			}

			/* Parse instruction, dependent on whether or not we are producing
			 * revert or commit code. */
			parseFrag( pd, code, stopId );
		}
	}

	/*
	 * Finish operation
	 */

	if ( !stop ) {
		code.append( IN_SEND_EOF_W );
		parseFrag( pd, code, stopId );
	}

	/* The parse is over: the code has its object again. */
	if ( lent )
		pd->removeOwn( lentRoot );

	if ( parserText->reduce ) {
		code.append( IN_REDUCE_COMMIT );
	}

	/* Pull out the error and save it off. */
	code.append( IN_DUP_VAL );
	code.append( IN_GET_PARSER_MEM_R );
	code.appendHalf( 1 );
	code.append( IN_SET_ERROR );

	/* Replace the parser with the parsed tree. */
	code.append( IN_GET_PARSER_MEM_R );
	code.appendHalf( 0 );
	popExprWords( pd, words );

	/* Capture to the local var. */
	evaluateCapture( pd, code, targetUT );

	return targetUT;
}

void LangTerm::evaluateSendStream( Compiler *pd, CodeVect &code ) const
{
	/* A send changes the stream. */
	pd->ctxValueInPlace = true;
	UniqueType *varUt = varRef->evaluate( pd, code );

	if ( varUt->listOf( pd->uniqueTypeStream ) ) {
		code.append( IN_GET_VLIST_MEM_R );
		code.appendHalf( varUt->generic->id );
		code.appendHalf( 0 );
	}

	/* The stream waits under its items. */
	long words = pd->exprWords.length();
	pushExprWord( pd, Compiler::ExprWord::Struct );

	/* Assign bind ids to the variables in the replacement. */
	for ( ConsItemList::Iter item = *parserText->list; item.lte(); item++ ) {
		uchar trim = TRIM_DEFAULT;

		switch ( item->type ) {
		case ConsItem::LiteralType: {
			String result;
			bool unusedCI;
			prepareLitString( result, unusedCI, 
					item->prodEl->typeRef->pdaLiteral->data,
					item->prodEl->typeRef->pdaLiteral->loc );

			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( result, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::InputText: {
			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( item->data, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::ExprType: {
			UniqueType *ut = item->expr->evaluate( pd, code );
			if ( ut->typeId == TYPE_VOID ) {
				/* Clear it away if return type is void. */
				code.append( IN_POP_VAL );
				continue;
			}

			checkSendItem( pd, item, ut, false, "only a tree, str, int or bool "
					"can be printed or sent to a stream" );

			if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
				code.append( IN_INT_TO_STR );

			if ( item->trim == ConsItem::TrimYes )
				trim = TRIM_YES;
			else if ( item->trim == ConsItem::TrimNo )
				trim = TRIM_NO;

			break;
		}}

		code.append( IN_PRINT_TREE );
		code.append( trim );
	}

	/* The stream is the send's value. */
	popExprWords( pd, words );
}

/* Gives whether a stream was sent: the parser's input holds it. */
bool LangTerm::evaluateSendParser( Compiler *pd, CodeVect &code, bool strings ) const
{
	bool sentStream = false;

	/* A send changes the parser. Sending to a parser variable isn't a use of
	 * it as a value. */
	pd->ctxValueInPlace = true;
	pd->sendTarget = true;
	UniqueType *varUt = varRef->evaluate( pd, code );

	if ( varUt->parser() ) {
	}
	else if ( varUt->listOf( pd->uniqueTypeStream ) ) {
		code.append( IN_GET_VLIST_MEM_R );
		code.appendHalf( varUt->generic->id );
		code.appendHalf( 0 );
	}

	/* The parser waits under its items and the parses. */
	long words = pd->exprWords.length();
	pushExprWord( pd, Compiler::ExprWord::Parser );

	if ( parserText->list->length() == 0 ) {
		code.append( IN_SEND_NOTHING );

		/* Parse instruction, dependent on whether or not we are producing
		 * revert or commit code. */
		parseFrag( pd, code, 0 );
	}
	else {
		/* Assign bind ids to the variables in the replacement. */
		for ( ConsItemList::Iter item = *parserText->list; item.lte(); item++ ) {
			bool isStream = false;
			uchar trim = TRIM_DEFAULT;

			switch ( item->type ) {
			case ConsItem::LiteralType: {
				String result;
				bool unusedCI;
				prepareLitString( result, unusedCI, 
						item->prodEl->typeRef->pdaLiteral->data,
						item->prodEl->typeRef->pdaLiteral->loc );

				/* Make sure we have this string. */
				StringMapEl *mapEl = 0;
				if ( pd->literalStrings.insert( result, &mapEl ) )
					mapEl->value = pd->literalStrings.length()-1;

				code.append( IN_LOAD_STR );
				code.appendWord( mapEl->value );
				break;
			}
			case ConsItem::InputText: {
				/* Make sure we have this string. */
				StringMapEl *mapEl = 0;
				if ( pd->literalStrings.insert( item->data, &mapEl ) )
					mapEl->value = pd->literalStrings.length()-1;

				code.append( IN_LOAD_STR );
				code.appendWord( mapEl->value );
				break;
			}
			case ConsItem::ExprType:
				UniqueType *ut = item->expr->evaluate( pd, code );
				if ( ut->typeId == TYPE_VOID ) {
					/* Clear it away if return type is void. */
					code.append( IN_POP_VAL );
					continue;
				}

				checkSendItem( pd, item, ut, true, "only a tree, str, int, bool "
						"or stream can be sent to a parser" );
				if ( !strings ) {
					checkTreeIntoParse( pd, item->expr, ut, "send_tree", "",
							"send its text" );
				}

				if ( ut == pd->uniqueTypeStream ) {
					isStream = true;
					sentStream = true;
				}

				if ( item->trim == ConsItem::TrimYes )
					trim = TRIM_YES;
				else if ( item->trim == ConsItem::TrimNo )
					trim = TRIM_NO;

				if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
					code.append( IN_INT_TO_STR );

				break;
			}

			if ( isStream )
				code.append( IN_SEND_STREAM_W );
			else if ( !strings ) {
				code.append( IN_SEND_TREE_W );
				code.append( trim );
			}
			else {
				code.append( IN_SEND_TEXT_W );
				code.append( trim );
			}

			parseFrag( pd, code, 0 );
		}
	}

	if ( eof ) {
		code.append( IN_SEND_EOF_W );
		parseFrag( pd, code, 0 );
	}

	/* The parser is the send's value. */
	popExprWords( pd, words );
	return sentStream;
}

/* A send to a parser runs its actions, and a stream sent to it, which its
 * input then holds, puts a reference value in. */
void LangTerm::checkSendChange( Compiler *pd, UniqueType *varUt, bool putsRef ) const
{
	/* What is sent to a stream isn't undone. */
	if ( varUt->parser() ) {
		varRef->noteGlobalUse( pd, GlobalUse::Change, String(), putsRef );
		pd->countOtherCode();
	}
}

/* A send that ends with eos finishes the parser. A send's value is the
 * parser: kept, it is a use of the parser as a value. */
void LangTerm::sendFinish( Compiler *pd, bool dropped ) const
{
	if ( varRef->qual->length() > 0 || !varRef->isLocalRef() )
		return;
	ObjectField *var = varRef->scope->findField( varRef->name );
	if ( var != 0 && !dropped )
		parserValueUse( pd, var, varRef->loc );
	if ( !eof )
		return;
	if ( var != 0 && pd->lentToParser( var ) && pd->throwaway == 0 ) {
		pd->releaseParser( var );
		pd->addOwn( var, Compiler::Own::Finished, varRef->loc );
	}
}

/* Nothing is sent to a parser after its finish(). */
void LangTerm::checkSendTarget( Compiler *pd ) const
{
	if ( varRef->qual->length() > 0 || !varRef->isLocalRef() )
		return;

	ObjectField *var = varRef->scope->findField( varRef->name );
	Compiler::Own *fin = var != 0 ? pd->findOwn( var ) : 0;
	if ( fin != 0 && fin->kind == Compiler::Own::Finished ) {
		std::ostream &out = error(varRef->loc) << "cannot send to " << var->name <<
				": it was finished at ";
		ownPlace( out, fin->loc );
		out << endp;
	}
}

UniqueType *LangTerm::evaluateSend( Compiler *pd, CodeVect &code ) const
{
	UniqueType *varUt = varRef->lookup( pd );
	bool sentStream = false;

	if ( varUt == pd->uniqueTypeStream )
		evaluateSendStream( pd, code );
	else if ( varUt->listOf( pd->uniqueTypeStream ) )
		evaluateSendStream( pd, code );
	else if ( varUt->parser() ) {
		bool dropped = pd->sendDropped;
		pd->sendDropped = false;
		checkSendTarget( pd );
		sentStream = evaluateSendParser( pd, code, true );
		sendFinish( pd, dropped );
	}
	else
		error(loc) << "can only send to parsers and streams" << endl;

	checkSendChange( pd, varUt, sentStream );
	return varUt;
}


UniqueType *LangTerm::evaluateSendTree( Compiler *pd, CodeVect &code ) const
{
	UniqueType *varUt = varRef->lookup( pd );
	bool sentStream = false;

	if ( varUt->parser() ) {
		bool dropped = pd->sendDropped;
		pd->sendDropped = false;
		checkSendTarget( pd );
		sentStream = evaluateSendParser( pd, code, false );
		sendFinish( pd, dropped );
	}
	else
		error(loc) << "can only send_tree to parsers" << endl;

	checkSendChange( pd, varUt, sentStream );
	return varUt;
}

UniqueType *LangTerm::evaluateEmbedString( Compiler *pd, CodeVect &code ) const
{
	/* Assign bind ids to the variables in the replacement. Each item gives a
	 * string, which waits under the items after it. */
	long words = pd->exprWords.length();
	for ( ConsItemList::Iter item = *consItemList; item.lte(); item++ ) {
		switch ( item->type ) {
		case ConsItem::LiteralType: {
			String result;
			bool unusedCI;
			prepareLitString( result, unusedCI, 
					item->prodEl->typeRef->pdaLiteral->data,
					item->prodEl->typeRef->pdaLiteral->loc );

			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( result, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::InputText: {
			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( item->data, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::ExprType: {
			UniqueType *ut = item->expr->evaluate( pd, code );

			if ( ut->typeId == TYPE_VOID ) {
				/* A void call gives nothing, as in a print. */
				code.append( IN_POP_VAL );

				String result = "";
				StringMapEl *mapEl = 0;
				if ( pd->literalStrings.insert( result, &mapEl ) )
					mapEl->value = pd->literalStrings.length()-1;

				code.append( IN_LOAD_STR );
				code.appendWord( mapEl->value );
				break;
			}

			checkSendItem( pd, item, ut, false, "only a tree, str, int or bool "
					"can go in a string" );

			if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
				code.append( IN_INT_TO_STR );

			/* A ref gives its tree's text, and nil gives NIL, as in a print. */
			if ( ( ut->typeId == TYPE_TREE && ut->langEl != pd->strLangEl ) ||
					ut->typeId == TYPE_REF || ut->typeId == TYPE_NIL )
			{
				/* Convert it to a string. */
				code.append( IN_TREE_TO_STR );
			}
			break;
		}}

		pushExprWord( pd, Compiler::ExprWord::Tree );
	}

	/* If there was nothing loaded, load the empty string. We must produce
	 * something. */
	if ( consItemList->length() == 0 ) {
		String result = "";

		/* Make sure we have this string. */
		StringMapEl *mapEl = 0;
		if ( pd->literalStrings.insert( result, &mapEl ) )
			mapEl->value = pd->literalStrings.length()-1;

		code.append( IN_LOAD_STR );
		code.appendWord( mapEl->value );
	}

	long items = consItemList->length();
	for ( long i = 0; i < items-1; i++ )
		code.append( IN_CONCAT_STR );
	popExprWords( pd, words );

	return pd->uniqueTypeStr;
}

UniqueType *LangTerm::evaluateSearch( Compiler *pd, CodeVect &code ) const
{
	UniqueType *ut = typeRef->uniqueType;
	if ( ut->typeId != TYPE_TREE )
		error(loc) << "can only search for tree types" << endp;

	/* Evaluate the expression. */
	UniqueType *treeUT = varRef->evaluate( pd, code );
	if ( !treeUT->treeValue() )
		error(loc) << "search can be applied only to tree/ref types" << endp;

	/* Run the search. */
	code.append( IN_TREE_SEARCH );
	code.appendWord( ut->langEl->id );
	return ut;
}

UniqueType *LangTerm::evaluate( Compiler *pd, CodeVect &code ) const
{
	UniqueType *retUt = 0;
	switch ( type ) {
		case VarRefType:
			retUt = varRef->evaluate( pd, code );
			break;
		case MethodCallType:
			retUt = varRef->evaluateCall( pd, code, args );
			break;
		case NilType:
			code.append( IN_LOAD_NIL );
			retUt = pd->uniqueTypeNil;
			break;
		case TrueType:
			code.append( IN_LOAD_TRUE );
			retUt = pd->uniqueTypeBool;
			break;
		case FalseType:
			code.append( IN_LOAD_FALSE );
			retUt = pd->uniqueTypeBool;
			break;
		case MakeTokenType:
			retUt = evaluateMakeToken( pd, code );
			break;
		case MakeTreeType:
			retUt = evaluateMakeTree( pd, code );
			break;
		case NumberType: {
			unsigned int n = atoi( data );
			code.append( IN_LOAD_INT );
			code.appendWord( n );
			retUt = pd->uniqueTypeInt;
			break;
		}
		case StringType: {
			String interp;
			bool unused;
			prepareLitString( interp, unused, data, InputLoc() );

			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( interp, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			retUt = pd->uniqueTypeStr;
			break;
		}
		case MatchType:
			retUt = evaluateMatch( pd, code );
			break;
		case ProdCompareType:
			retUt = evaluateProdCompare( pd, code );
			break;
		case ParseType:
			retUt = evaluateParse( pd, code, false, false );
			break;
		case ParseTreeType:
			retUt = evaluateParse( pd, code, true, false );
			break;
		case ParseStopType:
			retUt = evaluateParse( pd, code, false, true );
			break;
		case ConstructType:
			retUt = evaluateConstruct( pd, code );
			break;
		case SendType:
			retUt = evaluateSend( pd, code );
			break;
		case SendTreeType:
			retUt = evaluateSendTree( pd, code );
			break;
		case NewType:
			retUt = evaluateNew( pd, code );
			break;
		case TypeIdType: {
			/* Evaluate the expression. */
			UniqueType *ut = typeRef->uniqueType;
			if ( ut->typeId != TYPE_TREE )
				error(loc) << "typeid can only be applied to tree types" << endp;

			code.append( IN_LOAD_INT );
			code.appendWord( ut->langEl->id );
			retUt = pd->uniqueTypeInt;
			break;
		}
		case SearchType:
			retUt = evaluateSearch( pd, code );
			break;
		case EmbedStringType:
			retUt = evaluateEmbedString( pd, code );
			break;
		case CastType:
			retUt = evaluateCast( pd, code );
			break;
	}

	return retUt;
}

UniqueType *LangExpr::evaluate( Compiler *pd, CodeVect &code ) const
{
	switch ( type ) {
		case BinaryType: {
			switch ( op ) {
				case '+': {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					// pd->unwindCode.remove( 0, 2 );
					// pd->unwindCode.insert( 0, IN_POP_TREE );

					if ( lt == pd->uniqueTypeInt && rt == pd->uniqueTypeInt ) {
						code.append( IN_ADD_INT );
						return pd->uniqueTypeInt;
					}

					if ( lt == pd->uniqueTypeStr && rt == pd->uniqueTypeStr ) {
						code.append( IN_CONCAT_STR );
						return pd->uniqueTypeStr;
					}


					error(loc) << "do not have an addition operator for these types" << endp;
					break;
				}
				case '-': {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt == pd->uniqueTypeInt && rt == pd->uniqueTypeInt ) {
						code.append( IN_SUB_INT );
						return pd->uniqueTypeInt;
					}

					error(loc) << "do not have an addition operator for these types" << endp;
					break;
				}
				case '*': {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt == pd->uniqueTypeInt && rt == pd->uniqueTypeInt ) {
						code.append( IN_MULT_INT );
						return pd->uniqueTypeInt;
					}

					error(loc) << "do not have an multiplication "
							"operator for these types" << endp;
					break;
				}
				case '/': {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt == pd->uniqueTypeInt && rt == pd->uniqueTypeInt ) {
						code.append( IN_DIV_INT );
						return pd->uniqueTypeInt;
					}

					error(loc) << "do not have an division"
							"operator for these types" << endp;
					break;
				}
				case OP_DoubleEql: {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;
						
					if ( lt->ptr() || lt->scalar() )
						code.append( IN_TST_EQL_VAL );
					else
						code.append( IN_TST_EQL_TREE );
					return pd->uniqueTypeBool;
				}
				case OP_NotEql: {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->ptr() || lt->scalar() )
						code.append( IN_TST_NOT_EQL_VAL );
					else
						code.append( IN_TST_NOT_EQL_TREE );

					return pd->uniqueTypeBool;
				}
				case '<': {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->ptr() || lt->scalar() )
						code.append( IN_TST_LESS_VAL );
					else
						code.append( IN_TST_LESS_TREE );
					return pd->uniqueTypeBool;
				}
				case '>': {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->ptr() || lt->scalar() )
						code.append( IN_TST_GRTR_VAL );
					else
						code.append( IN_TST_GRTR_TREE );

					return pd->uniqueTypeBool;
				}
				case OP_LessEql: {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->ptr() || lt->scalar() )
						code.append( IN_TST_LESS_EQL_VAL );
					else
						code.append( IN_TST_LESS_EQL_TREE );

					return pd->uniqueTypeBool;
				}
				case OP_GrtrEql: {
					long words = pd->exprWords.length();
					UniqueType *lt = left->evaluate( pd, code );
					pushExprValue( pd, lt );
					UniqueType *rt = right->evaluate( pd, code );
					popExprWords( pd, words );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->ptr() || lt->scalar() )
						code.append( IN_TST_GRTR_EQL_VAL );
					else
						code.append( IN_TST_GRTR_EQL_TREE );

					return pd->uniqueTypeBool;
				}
				case OP_LogicalAnd: {
					/* Evaluate the left and duplicate it. */
					UniqueType *lut = left->evaluate( pd, code );
					if ( !( lut->ptr() || lut->scalar() ) )
						code.append( IN_TST_NZ_TREE );
					code.append( IN_DUP_VAL );

					/* Jump over the right if false, leaving the original left
					 * result on the top of the stack. We don't know the
					 * distance yet so record the position of the jump. */
					long jump = code.length();
					code.append( IN_JMP_FALSE_VAL );
					code.appendHalf( 0 );

					/* Evauluate the right, add the test. Store it separately.
					 * The left's test waits under it. */
					long words = pd->exprWords.length();
					pushExprWord( pd, Compiler::ExprWord::Value );
					UniqueType *rut = right->evaluate( pd, code );
					if ( !( rut->ptr() || rut->scalar() ) )
						code.append( IN_TST_NZ_TREE );
					popExprWords( pd, words );

					code.append( IN_TST_LOGICAL_AND );

					/* Set the distance of the jump. */
					long distance = code.length() - jump - 3;
					code.setHalf( jump+1, distance );

					return pd->uniqueTypeInt;
				}
				case OP_LogicalOr: {
					/* Evaluate the left and duplicate it. */
					UniqueType *lut = left->evaluate( pd, code );
					if ( !( lut->ptr() || lut->scalar() ) )
						code.append( IN_TST_NZ_TREE );
					code.append( IN_DUP_VAL );

					/* Jump over the right if true, leaving the original left
					 * result on the top of the stack. We don't know the
					 * distance yet so record the position of the jump. */
					long jump = code.length();
					code.append( IN_JMP_TRUE_VAL );
					code.appendHalf( 0 );

					/* Evauluate the right, add the test. The left's test
					 * waits under it. */
					long words = pd->exprWords.length();
					pushExprWord( pd, Compiler::ExprWord::Value );
					UniqueType *rut = right->evaluate( pd, code );
					if ( !( rut->ptr() || rut->scalar() ) )
						code.append( IN_TST_NZ_TREE );
					popExprWords( pd, words );

					code.append( IN_TST_LOGICAL_OR );

					/* Set the distance of the jump. */
					long distance = code.length() - jump - 3;
					code.setHalf( jump+1, distance );

					return pd->uniqueTypeInt;
				}
			}

			assert(false);
			return 0;
		}
		case UnaryType: {
			switch ( op ) {
				case '!': {
					/* Evaluate the left and duplicate it. */
					UniqueType *ut = right->evaluate( pd, code );
					if ( ut->ptr() || ut->scalar() )
						code.append( IN_NOT_VAL );
					else
						code.append( IN_NOT_TREE );
					return pd->uniqueTypeBool;
				}
				case '$': {
					UniqueType *ut = right->evaluate( pd, code );
					if ( !ut->treeValue() && !ut->scalar() && ut->typeId != TYPE_NIL )
						error(loc) << "$ needs a tree, an int or a bool" << endp;

					if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
						code.append( IN_INT_TO_STR );

					code.append( IN_TREE_TO_STR_TRIM );
					return pd->uniqueTypeStr;
					
				}
				case 'S': {
					UniqueType *ut = right->evaluate( pd, code );
					if ( !ut->treeValue() && !ut->scalar() && ut->typeId != TYPE_NIL )
						error(loc) << "$$ needs a tree, an int or a bool" << endp;

					if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
						code.append( IN_INT_TO_STR );

					code.append( IN_TREE_TO_STR_TRIM_A );
					return pd->uniqueTypeStr;
				}
				case '%': {
					UniqueType *ut = right->evaluate( pd, code );
					if ( !ut->treeValue() && !ut->scalar() && ut->typeId != TYPE_NIL )
						error(loc) << "% needs a tree, an int or a bool" << endp;
					if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
						code.append( IN_INT_TO_STR );
					else 
						code.append( IN_TREE_TO_STR );
					return pd->uniqueTypeStr;
				}
				case '^': {
					UniqueType *rt = right->evaluate( pd, code );
					if ( !rt->treeValue() && rt->typeId != TYPE_NIL )
						error(loc) << "^ needs a tree" << endp;
					code.append( IN_TREE_TRIM );
					return rt;
				}
				case '@': {
					UniqueType *rt = right->evaluate( pd, code );
					//code.append( IN_TREE_TRIM );
					return rt;
				}
				default: 
					assert(false);
			}
			return 0;
		}
		case TermType: {
			return term->evaluate( pd, code );
		}
	}
	return 0;
}

void LangVarRef::assignValue( Compiler *pd, CodeVect &code, 
		UniqueType *exprUT ) const
{
	/* Lookup the left hand side of the assignment. */
	VarRefLookup lookup = lookupField( pd );

	if ( lookup.objField->refActive )
		error(loc) << "reference active, cannot write to object" << endp;

	if ( lookup.firstConstPart >= 0 ) {
		error(loc) << "left hand side qualification \"" <<
			qual->data[lookup.firstConstPart].data << "\" is const" << endp;
	}

	if ( lookup.objField->isConst )
		error(loc) << "field \"" << name << "\" is const" << endp;

	noteGlobalUse( pd, GlobalUse::Change, String(), heldRef( pd, exprUT ) );

	/* A context variable named alone is a field of the running context. A
	 * local or a parameter given another object isn't used. */
	if ( qual->length() == 0 && !isLocalRef() )
		checkOwnedUse( pd );

	/* input and this, in a token action or a preeof block, are the parse's
	 * input and context. Nothing sets them: they have no set instructions. A
	 * qualified name is never one of them, so this->N = 1 still compiles. */
	if ( lookup.objField->isInbuiltObject() )
		error(loc) << "cannot assign to \"" << name << "\"" << endp;

	if ( qual->length() == 0 && genericLoopVar( lookup.objField ) ) {
		error(loc) << "cannot assign to " << name <<
				", the variable of a loop over a list or a map" << endp;
	}

	/* Writing guarantees the field is dirty. tree is dirty. */
	lookup.objField->dirtyTree = true;

	/* Check the types of the assignment and possibly cast. */
	UniqueType *objUT = lookup.objField->typeRef->uniqueType;
	assert( lookup.uniqueType == lookup.objField->typeRef->uniqueType );
	if ( !castAssignment( pd, code, objUT, lookup.iterSearchUT, exprUT ) )
		error(loc) << "type mismatch in assignment" << endp;
	
	/* Decide if we need to revert the assignment: as loadObj() loads the
	 * object, which for a child named on its own in a case is through the
	 * switch's variable. */
	bool revert = lookup.lastPtrInQual >= 0 || !isLocalRoot( pd );

	/* Load the object and generate the field setting code. */
	loadObj( pd, code, lookup.lastPtrInQual, true );

	if ( lookup.uniqueType->typeId == TYPE_ITER )
		setFieldIter( pd, code, lookup.inObject, lookup.objField, lookup.uniqueType, exprUT, false );
	else
		setField( pd, code, lookup.inObject, lookup.objField, exprUT, revert );
}

UniqueType *LangTerm::evaluateMakeToken( Compiler *pd, CodeVect &code ) const
{
//	if ( pd->compileContext != Compiler::CompileTranslation )
//		error(loc) << "make_token can be used only in a translation block" << endp;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs < 2 )
		error(loc) << "need at least two arguments" << endp;

	/* An il holds ignored text in its children and has no text of its own,
	 * so make_token can't make one. An id that isn't typeid<il> written out
	 * goes unchecked. */
	LangExpr *idExpr = args->data[0]->expr;
	if ( idExpr->type == LangExpr::TermType &&
			idExpr->term->type == LangTerm::TypeIdType &&
			idExpr->term->typeRef->uniqueType->langEl == pd->ignoreLangEl )
	{
		error(loc) << "make_token cannot make an il" << endp;
	}

	/* Each argument waits under the ones after it. */
	long words = pd->exprWords.length();
	for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
		/* Evaluate. */
		UniqueType *exprUT = (*pe)->expr->evaluate( pd, code );

		if ( pe.pos() == 0 && exprUT != pd->uniqueTypeInt )
			error(loc) << "first arg, id, must be an int" << endp;

		if ( pe.pos() == 1 && exprUT != pd->uniqueTypeStr )
			error(loc) << "second arg, length, must be a string" << endp;

		/* A struct or a generic put into an attribute is held strongly,
		 * whatever the attribute's type: checkStrongCycles() counts it. */
		if ( pe.pos() >= 2 && exprUT->ptr() ) {
			Compiler::MakeTokenArg arg;
			arg.loc = loc;
			arg.ut = exprUT;
			pd->makeTokenArgs.append( arg );
		}

		/* The rest are attributes. A value goes into one wrapped, as an
		 * assignment stores it. */
		if ( pe.pos() >= 2 && ( exprUT->ptr() || exprUT->scalar() ) ) {
			code.append( IN_VAL_TO_POINTER );
			pushExprWord( pd, Compiler::ExprWord::Tree );
		}
		else {
			pushExprValue( pd, exprUT );
		}
	}

	/* The token is now created, send it. */
	code.append( IN_MAKE_TOKEN );
	code.append( args->length() );
	popExprWords( pd, words );

	return pd->uniqueTypeAny;
}

UniqueType *LangTerm::evaluateMakeTree( Compiler *pd, CodeVect &code ) const
{
//	if ( pd->compileContext != Compiler::CompileTranslation )
//		error(loc) << "make_tree can be used only in a translation block" << endp;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs < 1 )
		error(loc) << "need at least one argument" << endp;

	/* make_tree makes a def, or an il, whose text is in its children. A token
	 * or a str has text of its own, which make_tree can't give it. An id that
	 * isn't typeid<T> written out is checked when it runs. A typeid of a type
	 * that isn't a tree has no lang el, and evaluating it reports that. */
	LangExpr *idExpr = args->data[0]->expr;
	if ( idExpr->type == LangExpr::TermType &&
			idExpr->term->type == LangTerm::TypeIdType &&
			idExpr->term->typeRef->uniqueType->typeId == TYPE_TREE )
	{
		LangEl *langEl = idExpr->term->typeRef->uniqueType->langEl;
		if ( langEl->type == LangEl::Term && langEl != pd->ignoreLangEl )
			error(loc) << "make_tree cannot make a token or str" << endp;
	}

	/* Each argument waits under the ones after it. */
	long words = pd->exprWords.length();
	for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
		/* Evaluate. */
		UniqueType *exprUT = (*pe)->expr->evaluate( pd, code );

		if ( pe.pos() == 0 && exprUT != pd->uniqueTypeInt )
			error(loc) << "first arg, nonterm id, must be an int" << endp;

		pushExprValue( pd, exprUT );
	}

	/* The token is now created, send it. */
	code.append( IN_MAKE_TREE );
	code.append( args->length() );
	popExprWords( pd, words );

	return pd->uniqueTypeAny;
}

/* A list of statements, whose variables are declared in scope. What is
 * recorded of them goes with the list: nothing can send to a parser a
 * variable of the list holds once the list has ended. */
static void compileStmtList( Compiler *pd, CodeVect &code, StmtList *stmtList,
		NameScope *scope )
{
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->compile( pd, code );

	if ( scope != 0 )
		pd->endScopeOwns( scope, false );
}

/* A loop's body runs again after its end. What is outside the loop must be
 * as the body found it: start, taken before the body, and a while loop's
 * condition. */
static void compileLoopBody( Compiler *pd, CodeVect &code, StmtList *stmtList,
		NameScope *scope, const Vector<Compiler::Own> &start, const InputLoc &loopLoc )
{
	pd->loopDepth += 1;
	compileStmtList( pd, code, stmtList, scope );
	pd->checkLoopOwns( start, loopLoc );
	pd->loopDepth -= 1;
}

void LangStmt::compileForIterBody( Compiler *pd, 
		CodeVect &code, UniqueType *iterUT, const CodeVect &under ) const
{
	/* Add the cleanup for the current loop: the iterator's unwind, then the
	 * words under the iterator. */
	int lcLen = pd->unwindCode.length();
	CodeVect leave;
	leave.append( objField->iterImpl->inUnwind );
	leave.appendHalf( objField->offset );
	leave.append( under );
	pd->unwindCode.insert( 0, leave );

	/* Remember the top of the loop. */
	long top = code.length();

	/* Advance */
	code.append( objField->iterImpl->inAdvance );
	code.appendHalf( objField->offset );

	/* A user iterator's own code runs at the advance. An exit in it comes
	 * back to the loop, whose unwind code ends the iterator. */
	if ( objField->iterImpl->type == IterImpl::User )
		appendUnwindCode( pd, code, UnwindOther );

	/* Test: jump past the while block if false. Note that we don't have the
	 * distance yet. */
	long jumpFalse = code.length();
	code.append( IN_JMP_FALSE_VAL );
	code.appendHalf( 0 );

	/* Compile the contents. Its breaks are its own. In it, the words of an
	 * expression lie on what this loop's iterator left. */
	LongVect outerBreaks = pd->breakJumps;
	pd->breakJumps.empty();
	Vector< Vector<Compiler::Own> > outerBreakOwns = pd->breakOwns;
	pd->breakOwns.empty();
	ObjectField *outerLoop = pd->unwindLoop;
	pd->unwindLoop = objField;

	/* The loop ends at the advance, before the body, or at a break. */
	Vector<Compiler::Own> start = pd->owns;
	compileLoopBody( pd, code, stmtList, scope, start, loc );
	pd->leaveLoop( start, scope );
	pd->breakOwns = outerBreakOwns;

	pd->unwindLoop = outerLoop;
	pd->unwindCode.remove( 0, pd->unwindCode.length() - lcLen );

	/* Jump back to the top to retest. */
	long retestDist = code.length() - top + 3;
	code.append( IN_JMP );
	code.appendHalf( -retestDist );

	/* Set the jump false distance. */
	long falseDist = code.length() - jumpFalse - 3;
	code.setHalf( jumpFalse+1, falseDist );

	/* Compute the jump distance for the break jumps. */
	for ( LongVect::Iter brk = pd->breakJumps; brk.lte(); brk++ ) {
		long distance = code.length() - *brk - 3;
		code.setHalf( *brk+1, distance );
	}
	pd->breakJumps = outerBreaks;

	/* Destroy the iterator, then pop the words under it. */
	code.append( objField->iterImpl->inDestroy );
	code.appendHalf( objField->offset );
	code.append( under );
}

void LangStmt::compileForIter( Compiler *pd, CodeVect &code ) const
{
	/* The type we are searching for. */
	UniqueType *searchUT = typeRef->uniqueType;

	/* Lookup the iterator call. Make sure it is an iterator. */
	VarRefLookup lookup = iterCall->langTerm->varRef->lookupIterCall( pd );
	if ( lookup.objMethod->iterDef == 0 ) {
		error(loc) << "attempt to iterate using something "
				"that is not an iterator" << endp;
	}

	/* Prepare the contiguous call args space. */
	Function *func = lookup.objMethod->func;
	int asLoc;
	long words = pd->exprWords.length();
	if ( func != 0 ) {
		code.append( IN_PREP_ARGS );
		asLoc = code.length();
		code.appendHalf( 0 );
		pushExprArea( pd, func->paramListSize );
	}

	/* 
	 * Create the iterator from the local var.
	 */

	UniqueType *iterUT = objField->typeRef->uniqueType;
	IterImpl *iterImpl = 0;

	switch ( iterUT->iterDef->type ) {
		case IterDef::Tree:
			iterImpl = iterCall->langTerm->varRef->chooseTriterCall( pd,
					searchUT, iterCall->langTerm->args );
			break;
		case IterDef::WithIgnore:
			iterImpl = new IterImpl( IterImpl::WithIgnore );
			break;
		case IterDef::Child:
			iterImpl = new IterImpl( IterImpl::Child );
			break;
		case IterDef::RevChild:
			iterImpl = new IterImpl( IterImpl::RevChild );
			break;
		case IterDef::Repeat:
			iterImpl = new IterImpl( IterImpl::Repeat );
			break;
		case IterDef::RevRepeat:
			iterImpl = new IterImpl( IterImpl::RevRepeat );
			break;
		case IterDef::User:
			iterImpl = new IterImpl( IterImpl::User, iterUT->iterDef->func );
			break;
		case IterDef::RevListVal:
			iterImpl = new IterImpl( IterImpl::RevListVal );
			break;
		case IterDef::MapKey:
			iterImpl = new IterImpl( IterImpl::MapKey );
			break;
	}

	objField->iterImpl = iterImpl;

	/* A loop over a tree or a user iterator selects the trees of the loop
	 * variable's type, by the type's id. A type that isn't a tree has none. */
	if ( iterImpl->useSearchUT && searchUT->langEl == 0 ) {
		error(loc) << "loop variable " << objField->name <<
				" must have a tree type" << endp;
	}

	/* A loop over the values of a list or a map that a context object holds
	 * reads it in place. */
	bool inPlace = iterImpl->useGenericId &&
			forInPlace( pd, searchUT, iterCall->langTerm->args );

	/* A user iterator is other code. */
	if ( func != 0 )
		pd->countOtherCode();

	noteLentCall( pd, func, iterCall->langTerm->varRef->loc );
	Vector<LangVarRef*> ctxArgs;
	checkContextArgs( pd, func, lookup.objMethod->paramUTs,
			lookup.objMethod->numParams, iterCall->langTerm->args, ctxArgs );

	if ( func != 0 && pd->revertOn && pd->curBlock != 0 )
		pd->curBlock->calls.append( func );

	/* Evaluate and push the arguments. */
	pd->ctxValueInPlace = inPlace;
	ObjectField **paramRefs = iterCall->langTerm->varRef->evaluateArgs(
			pd, code, lookup, iterCall->langTerm->args );
	pd->ctxValueInPlace = false;
	checkContextArgsKept( pd, func, ctxArgs );

	/* A tree iterator walks a tree. Its parameter is a ref<any>, which takes
	 * any argument, so the argument is checked here. A loop that names no
	 * iterator over something other than a list or a map gets the tree
	 * iterator too. */
	if ( iterImpl->type == IterImpl::Tree || iterImpl->type == IterImpl::WithIgnore ||
			iterImpl->type == IterImpl::Child || iterImpl->type == IterImpl::RevChild ||
			iterImpl->type == IterImpl::Repeat || iterImpl->type == IterImpl::RevRepeat )
	{
		LangVarRef *iterRef = iterCall->langTerm->varRef;
		CodeVect unused;
		pd->throwaway += 1;
		UniqueType *argUT =
				iterCall->langTerm->args->data[0]->expr->evaluate( pd, unused );
		pd->throwaway -= 1;
		if ( !argUT->treeValue() ) {
			if ( iterCall->wasExpr ) {
				error(iterRef->loc) << "can only loop over a tree, "
						"a list or a map" << endp;
			}
			error(iterRef->loc) << iterRef->name << " takes a tree" << endp;
		}

		/* The loop variable is in the tree the loop walks. A write through
		 * it is a write through the variable the argument starts at. A loop
		 * over a global, a struct's field or a context's variable walks a
		 * copy, so its variable is read-only: checkRefCopies() checks it. */
		LangExpr *rootExpr = iterCall->langTerm->args->data[0]->expr;
		if ( rootExpr->canTakeRef( pd ) ) {
			VarRefLookup rootLookup = rootExpr->term->varRef->lookupField( pd );
			pd->refFlow( &rootLookup.rootField->refWrite, &objField->refWrite );
		}
		else if ( rootExpr->type == LangExpr::TermType &&
				rootExpr->term->type == LangTerm::VarRefType )
		{
			Compiler::RefCopy copy;
			copy.loc = loc;
			copy.argPos = 0;
			copy.func = 0;
			copy.param = 0;
			copy.loopVar = objField;
			pd->refCopies.append( copy );
		}
	}

	/* The variable of a loop over a user iterator holds what it yields. */
	if ( func != 0 )
		pd->refFlow( &func->yieldWrite, &objField->refWrite );

	/* A loop over a list or a map. Its iterator walks one kind, and gives the
	 * values or the keys as they are stored. So the argument must be of that
	 * kind, and the loop variable must take what the iterator gives. */
	GenericType *generic = 0;
	if ( iterImpl->useGenericId ) {
		bool overMap = iterImpl->type == IterImpl::MapVal ||
				iterImpl->type == IterImpl::MapKey;
		const char *kind = overMap ? "map" : "list";

		LangVarRef *iterRef = iterCall->langTerm->varRef;
		CodeVect unused;
		pd->ctxValueInPlace = inPlace;
		pd->throwaway += 1;
		UniqueType *argUT =
				iterCall->langTerm->args->data[0]->expr->evaluate( pd, unused );
		pd->throwaway -= 1;
		pd->ctxValueInPlace = false;
		if ( argUT->typeId != TYPE_GENERIC ||
				argUT->generic->typeId != ( overMap ? GEN_MAP : GEN_LIST ) )
		{
			error(iterRef->loc) << iterRef->name << " takes a " << kind << endp;
		}

		generic = argUT->generic;

		/* The loop gives each value or key as a strong reference: a weak
		 * value's is its target. So the variable has that type, over keys and
		 * strong values as over weak values. */
		if ( searchUT->weak() ) {
			error(loc) << "loop variable " << objField->name <<
					" can't be weak: the loop gives each value as a strong reference" << endp;
		}

		if ( iterImpl->type == IterImpl::MapKey ) {
			if ( !castAssignment( pd, unused, searchUT, 0, generic->keyUt ) ) {
				error(loc) << "loop variable " << objField->name <<
						" must have the map's key type" << endp;
			}
		}
		else if ( !castAssignment( pd, unused, searchUT, 0,
				generic->valueUt->readType() ) )
		{
			error(loc) << "loop variable " << objField->name <<
					" must have the " << kind << "'s value type" << endp;
		}
	}

	if ( pd->revertOn )
		code.append( iterImpl->inCreateWV );
	else
		code.append( iterImpl->inCreateWC );

	code.appendHalf( objField->offset );

	/* Arg size (or func id for user iters). */
	if ( lookup.objMethod->func != 0 )
		code.appendHalf( lookup.objMethod->func->funcId );
	else
		code.appendHalf( iterCall->langTerm->varRef->argSize );

	/* Search type. */
	if ( iterImpl->useSearchUT )
		code.appendHalf( searchUT->langEl->id );

	if ( iterImpl->useGenericId )
		code.appendHalf( generic->id );

	/* The iterator has its arguments. What it leaves of them is under the
	 * loop, whose unwind code pops it. */
	popExprWords( pd, words );

	/* The words the loop leaves under its iterator: the refs of a ref
	 * argument's qualification, and a user iterator's arguments. The end of
	 * the loop pops them, and so does a return or an exit that leaves it. */
	CodeVect under;
	iterCall->langTerm->varRef->popRefQuals( pd, under, lookup,
			iterCall->langTerm->args, false );
	if ( func != 0 ) {
		under.append( IN_CLEAR_ARGS );
		under.appendHalf( func->paramListSize );
		code.setHalf( asLoc, func->paramListSize );
	}

	/* A user iterator's context argument is lent to it for the loop, whose
	 * body runs while the iterator is suspended. */
	Vector<ObjectField*> lentRoots;
	if ( func != 0 && iterCall->langTerm->args != 0 ) {
		for ( int a = 0; a < iterCall->langTerm->args->length(); a++ ) {
			UniqueType *paramUT = lookup.objMethod->paramUTs[a];
			LangExpr *argExpr = iterCall->langTerm->args->data[a]->expr;
			ObjectField *root;
			if ( paramUT->typeId == TYPE_STRUCT && paramUT->structEl->structDef->isContext &&
					argExpr->type == LangExpr::TermType &&
					argExpr->term->type == LangTerm::VarRefType &&
					argExpr->term->varRef->ownRoot( pd, root ) )
			{
				pd->addOwn( root, Compiler::Own::LentToLoop,
						iterCall->langTerm->varRef->loc );
				pd->findOwn( root )->iter = iterCall->langTerm->varRef->name;
				lentRoots.append( root );
			}
		}
	}

	compileForIterBody( pd, code, iterUT, under );

	for ( int r = 0; r < lentRoots.length(); r++ )
		pd->removeOwn( lentRoots[r] );

	iterCall->langTerm->varRef->resetActiveRefs( pd, lookup, paramRefs );
	delete[] paramRefs;
}

void LangStmt::compileWhile( Compiler *pd, CodeVect &code ) const
{
	/* Generate code for the while test. Remember the top. The test runs
	 * again after the body, so what is outside the loop must be as the test
	 * found it. */
	long top = code.length();
	Vector<Compiler::Own> start = pd->owns;
	UniqueType *eut = expr->evaluate( pd, code );
	Vector<Compiler::Own> test = pd->owns;

	/* Jump past the while block if false. Note that we don't have the
	 * distance yet. */
	long jumpFalse = code.length();
	half_t jinstr = eut->treeValue() ? IN_JMP_FALSE_TREE : IN_JMP_FALSE_VAL;
	code.append( jinstr );
	code.appendHalf( 0 );

	/* Compute the while block. Its breaks are its own. The loop ends where
	 * the test fails, or at a break. */
	LongVect outerBreaks = pd->breakJumps;
	pd->breakJumps.empty();
	Vector< Vector<Compiler::Own> > outerBreakOwns = pd->breakOwns;
	pd->breakOwns.empty();

	compileLoopBody( pd, code, stmtList, scope, start, loc );
	pd->leaveLoop( test, scope );
	pd->breakOwns = outerBreakOwns;

	/* Jump back to the top to retest. */
	long retestDist = code.length() - top + 3;
	code.append( IN_JMP );
	code.appendHalf( -retestDist );

	/* Set the jump false distance. */
	long falseDist = code.length() - jumpFalse - 3;
	code.setHalf( jumpFalse+1, falseDist );

	/* Compute the jump distance for the break jumps. */
	for ( LongVect::Iter brk = pd->breakJumps; brk.lte(); brk++ ) {
		long distance = code.length() - *brk - 3;
		code.setHalf( *brk+1, distance );
	}
	pd->breakJumps = outerBreaks;
}

void LangStmt::compile( Compiler *pd, CodeVect &code ) const
{
	/* A statement consumes what it pushes. */
	long words = pd->exprWords.length();

	switch ( type ) {
		case ExprType: {
			/* Evaluate the exrepssion, then pop it immediately. A send's
			 * value, its parser, goes nowhere. */
			pd->sendDropped = expr->type == LangExpr::TermType &&
					( expr->term->type == LangTerm::SendType ||
					expr->term->type == LangTerm::SendTreeType );
			UniqueType *exprUt = expr->evaluate( pd, code );
			pd->sendDropped = false;
			if ( exprUt->treeValue() )
				code.append( IN_POP_TREE );
			else
				code.append( IN_POP_VAL );

			// pd->unwindCode.remove( 0, 1 );
			break;
		}
		case IfType: {
			long jumpFalse = 0, jumpPastElse = 0, distance = 0;

			/* Evaluate the test. */
			UniqueType *eut = expr->evaluate( pd, code );

			/* Jump past the if block if false. We don't know the distance
			 * yet so store the location of the jump. */
			jumpFalse = code.length();
			half_t jinstr = eut->treeValue() ? IN_JMP_FALSE_TREE : IN_JMP_FALSE_VAL;

			code.append( jinstr );
			code.appendHalf( 0 );

			/* Compile the if true branch, from what the test left. */
			Vector<Compiler::Own> before = pd->owns;
			compileStmtList( pd, code, stmtList, scope );

			if ( elsePart != 0 ) {
				/* Jump past the else code for the if true branch. */
				jumpPastElse = code.length();
				code.append( IN_JMP );
				code.appendHalf( 0 );
			}

			/* Set the distance for the jump false case. */
			distance = code.length() - jumpFalse - 3;
			code.setHalf( jumpFalse+1, distance );

			/* The else branch starts from what the test left too. After
			 * the if, what either branch did holds. */
			Vector<Compiler::Own> afterThen = pd->owns;
			pd->owns = before;
			if ( elsePart != 0 ) {
				/* Compile the else branch. */
				elsePart->compile( pd, code );

				/* Set the distance for jump over the else part. */
				distance = code.length() - jumpPastElse - 3;
				code.setHalf( jumpPastElse+1, distance );
			}
			pd->mergeOwns( afterThen );

			break;
		}
		case ElseType: {
			/* Compile the else branch. */
			compileStmtList( pd, code, stmtList, scope );
			break;
		}
		case RejectType: {
			/* Only a production's action can reject. A function may be called
			 * from one, so the run time checks those. */
			switch ( pd->compileContext ) {
				case Compiler::CompileTranslation:
					error(loc) << "reject in a token action" << endp;
					break;
				case Compiler::CompilePreEof:
					error(loc) << "reject in a preeof block" << endp;
					break;
				case Compiler::CompileRoot:
					error(loc) << "reject outside a production action" << endp;
					break;
				case Compiler::CompileReduction:
				case Compiler::CompileFunction:
					break;
			}

			code.append( IN_REJECT );
			break;
		}
		case WhileType: {
			compileWhile( pd, code );
			break;
		}
		case AssignType: {
			/* The local variable assigned to, if that is what it is. */
			VarRefLookup lookup = varRef->lookupField( pd );
			ObjectField *var = varRef->qual->length() == 0 && varRef->isLocalRef() ?
					lookup.objField : 0;

			/* A new parser that goes into a parser variable is lent its
			 * context there. */
			bool newParser = var != 0 && var->typeRef->uniqueType->parser() &&
					expr->type == LangExpr::TermType &&
					expr->term->type == LangTerm::NewType;
			pd->newParserVar = newParser ? var : 0;
			pd->newParserLent = false;

			/* Evaluate the exrepssion. A change through a context reference
			 * puts it into the object. */
			setCtxStore( pd, varRef, lookup );
			UniqueType *exprUT = evaluateSetupValue( pd, code, expr );
			pd->ctxStore = false;
			pd->newParserVar = 0;

			noteMove( pd, expr );

			/* Do the assignment. */
			varRef->assignValue( pd, code, exprUT );

			/* The variable given another value ends what was recorded of
			 * the one it had. */
			if ( var != 0 ) {
				pd->releaseParser( var );
				pd->removeOwn( var );
			}

			if ( newParser && pd->newParserLent ) {
				pd->addOwn( pd->newParserRoot, Compiler::Own::LentToParser,
						pd->newParserLoc, var );
				pd->findOwn( pd->newParserRoot )->field = pd->newParserField;
				pd->newParserLent = false;
			}
			break;
		}
		case ForIterType: {
			compileForIter( pd, code );
			break;
		}
		case ReturnType: {
			/* Only a function or the main code can return. */
			switch ( pd->compileContext ) {
				case Compiler::CompileReduction:
					error(loc) << "return in a production action" << endp;
					break;
				case Compiler::CompileTranslation:
					error(loc) << "return in a token action" << endp;
					break;
				case Compiler::CompilePreEof:
					error(loc) << "return in a preeof block" << endp;
					break;
				case Compiler::CompileFunction:
					if ( pd->curFunction->isUserIter )
						error(loc) << "return in an iterator" << endp;
					break;
				case Compiler::CompileRoot:
					break;
			}

			/* Evaluate the exrepssion. */
			UniqueType *exprUT = expr->evaluate( pd, code );
			noteMove( pd, expr );

			if ( pd->curFunction == 0 ) {
				/* In the main function */
				pd->mainReturnUT = exprUT;
			}
			else {
				/* A void function returns nothing: nil, which ends it early,
				 * or the nothing a void call gives. */
				UniqueType *resUT = pd->curFunction->typeRef->uniqueType;
				bool voidOk = exprUT == pd->uniqueTypeNil ||
						exprUT == pd->uniqueTypeVoid;
				if ( resUT == pd->uniqueTypeVoid ? !voidOk :
						!castAssignment( pd, code, resUT, 0, exprUT ) )
					error(loc) << "return value wrong type" << endp;
			}

			code.append( IN_SAVE_RET );

			/* The loop cleanup code. */
			if ( pd->unwindCode.length() > 0 )
				code.append( pd->unwindCode );

			/* Jump to the return label. The distance will be filled in
			 * later. */
			pd->returnJumps.append( code.length() );
			code.append( IN_JMP );
			code.appendHalf( 0 );
			break;
		}
		case BreakType: {
			/* A break leaves the innermost loop of the code block. */
			if ( pd->loopDepth == 0 )
				error(loc) << "break outside a loop" << endp;

			/* The loop is left with what is lent or moved here. */
			pd->breakJumps.append( code.length() );
			pd->breakOwns.append( pd->owns );
			code.append( IN_JMP );
			code.appendHalf( 0 );
			break;
		}
		case YieldType: {
			/* Only a function can yield: an iterator, or a function an
			 * iterator calls. IN_YIELD stops a function that no iterator
			 * called. */
			switch ( pd->compileContext ) {
				case Compiler::CompileReduction:
					error(loc) << "yield in a production action" << endp;
					break;
				case Compiler::CompileTranslation:
					error(loc) << "yield in a token action" << endp;
					break;
				case Compiler::CompilePreEof:
					error(loc) << "yield in a preeof block" << endp;
					break;
				case Compiler::CompileRoot:
					error(loc) << "yield at the top level" << endp;
					break;
				case Compiler::CompileFunction:
					break;
			}

			/* A yield runs the loop's body, which can use the running
			 * context, and an object a parameter reaches. A local of this
			 * function it can't reach. */
			pd->countOtherCode();
			if ( pd->revertOn && pd->curBlock != 0 )
				pd->curBlock->yields = true;
			for ( int i = 0; i < pd->owns.length(); i++ ) {
				Compiler::Own &own = pd->owns[i];
				bool lent = own.kind == Compiler::Own::LentToParse ||
						own.kind == Compiler::Own::LentToParser ||
						own.kind == Compiler::Own::LentToLoop;
				if ( lent && ( own.var == 0 || own.var->isParam() ) ) {
					std::ostream &out = error(loc) << "cannot yield while " <<
							( own.var == 0 ? String( "the context object" ) : own.var->name ) <<
							" is lent (";
					ownPlace( out, own.loc );
					out << "): the loop's body could use it" << endp;
				}
			}

			/* take a reference and yield it. Immediately reset the referece. */
			ObjectField *objField = varRef->preEvaluateRef( pd, code );

			/* A loop takes what an iterator yields as a tree. A loop
			 * variable holds a value of its loop's search type. */
			TypeRef *yieldTR = objField->typeRef;
			UniqueType *yieldUT = yieldTR->searchUniqueType != 0 ?
					yieldTR->searchUniqueType : yieldTR->uniqueType;
			if ( yieldUT->langEl == 0 )
				error(varRef->loc) << "can only yield a tree" << endp;

			/* Checked here, before evaluateRef refuses the reference, so the
			 * error says it is the yield. */
			if ( varRef->qual->length() == 0 && genericLoopVar( objField ) ) {
				error(varRef->loc) << "cannot yield " << objField->name <<
						", the variable of a loop over a list or a map" << endp;
			}

			/* A write through the loop's variable is a write through the
			 * variable the yield starts at. */
			VarRefLookup yieldLookup = varRef->lookupField( pd );
			pd->refFlow( &yieldLookup.rootField->refWrite,
					&pd->curFunction->yieldWrite );

			varRef->evaluateRef( pd, code, 0 );

			code.append( IN_YIELD );

			/* Leaving the loop the iterator yields to ends the iterator
			 * from here, with this unwind code: the refs of the yield's
			 * qualification, then the loops the yield is in. */
			{
				CodeVect quals;
				if ( varRef->qual->length() > 0 ) {
					quals.append( IN_POP_N_WORDS );
					quals.appendHalf( (short)(varRef->qual->length()*2) );
				}
				pd->unwindCode.insert( 0, quals );
				appendUnwindCode( pd, code, UnwindOther );
				pd->unwindCode.remove( 0, quals.length() );
			}

			if ( varRef->qual->length() > 0 ) {
				code.append( IN_POP_N_WORDS );
				code.appendHalf( (short)(varRef->qual->length()*2) );
			}

			objField->refActive = false;
			break;
		}
	}

	assert( pd->exprWords.length() == words );
}

void CodeBlock::compile( Compiler *pd, CodeVect &code ) const
{
	/* A code block's ownership state and loops are its own. A function or an
	 * iterator starts with none: its caller lends its parameters. */
	Vector<Compiler::Own> outer = pd->owns;
	int outerLoopDepth = pd->loopDepth;
	ObjectField *outerLoop = pd->unwindLoop;
	pd->owns.empty();
	pd->loopDepth = 0;
	pd->unwindLoop = 0;

	compileStmtList( pd, code, stmtList, 0 );

	pd->owns = outer;
	pd->loopDepth = outerLoopDepth;
	pd->unwindLoop = outerLoop;
}

void Compiler::findLocals( ObjectDef *localFrame, CodeBlock *block )
{
	Locals &locals = block->locals;

	for ( FieldList::Iter ol = localFrame->fieldList; ol.lte(); ol++ ) {
		ObjectField *el = ol->value;

		/* FIXME: This test needs to be improved. Match_text was getting
		 * through before useOffset was tested. What will? */

		/* A reduction's lhs is listed too. The end of the action moves it
		 * back to the parser and clears the local, but an exit in the action
		 * leaves it in the local. */
		if ( el->useOffset() && ( el->beenReferenced || el->isParam() ) ) {
			UniqueType *ut = el->typeRef->uniqueType;
			if ( ut->tree() ) {
				int depth = el->scope->depth();
				locals.append( LocalLoc( LT_Tree, depth, el->offset ) );
			}
		}
	}
}

void Compiler::addProdLHSLoad( Production *prod, CodeVect &code, long &insertPos )
{
	NameScope *scope = prod->redBlock->localFrame->rootScope;
	ObjectField *lhsField = scope->findField("lhs");
	assert( lhsField != 0 );

	CodeVect loads;
	if ( lhsField->beenReferenced ) {
		loads.append( IN_INIT_LHS_EL );
		loads.appendHalf( lhsField->offset );
	}

	code.insert( insertPos, loads );
	insertPos += loads.length();
}

void Compiler::addPushBackLHS( Production *prod, CodeVect &code, long &insertPos )
{
	CodeBlock *block = prod->redBlock;

	/* If the lhs tree is dirty then we will need to save off the old lhs
	 * before it gets modified. We want to avoid this for attribute
	 * modifications. The computation of dirtyTree should deal with this for
	 * us. */
	NameScope *scope = block->localFrame->rootScope;
	ObjectField *lhsField = scope->findField("lhs");
	assert( lhsField != 0 );

	if ( lhsField->beenReferenced ) {
		code.append( IN_STORE_LHS_EL );
		code.appendHalf( lhsField->offset );
	}
}

void Compiler::addProdRHSLoads( Production *prod, CodeVect &code, long &insertPos )
{
	CodeVect loads;
	long elPos = 0;
	for ( ProdElList::Iter rhsEl = *prod->prodElList; rhsEl.lte(); rhsEl++, elPos++ ) {
		if ( rhsEl->type == ProdEl::ReferenceType ) {
			if ( rhsEl->rhsElField->beenReferenced ) {
				loads.append ( IN_INIT_RHS_EL );
				loads.appendHalf( elPos );
				loads.appendHalf( rhsEl->rhsElField->offset );
			}
		}
	}

	/* Insert and update the insert position. */
	code.insert( insertPos, loads );
	insertPos += loads.length();
}



void Compiler::makeProdCopies( Production *prod )
{
	int pos = 0;
	for ( ProdElList::Iter pel = *prod->prodElList; pel.lte(); pel++, pos++) {
		if ( pel->captureField != 0 ) {
			prod->copy.append( pel->captureField->offset );
			prod->copy.append( pos );
		}
	}
}

void Compiler::compileReductionCode( Production *prod )
{
	CodeBlock *block = prod->redBlock;

	/* Init the compilation context. */
	compileContext = CompileReduction;
	curFunction = 0;
	revertOn = true;
	curBlock = block;
	block->frameId = nextFrameId++;

	CodeVect &code = block->codeWV;

	long afterInit = code.length();

	/* Compile the reduce block. */
	block->compile( this, code );

	/* Might need to load right hand side values. */
	addProdRHSLoads( prod, code, afterInit );

	addProdLHSLoad( prod, code, afterInit );
	addPushBackLHS( prod, code, afterInit );

	code.append( IN_PCR_RET );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

void Compiler::compileTranslateBlock( LangEl *langEl )
{
	CodeBlock *block = langEl->transBlock;

	/* Set up compilation context. */
	compileContext = CompileTranslation;
	curFunction = 0;
	revertOn = true;
	curBlock = block;
	block->frameId = nextFrameId++;

	CodeVect &code = block->codeWV;

	if ( langEl->tokenDef->reCaptureVect.length() > 0 ) {
		code.append( IN_INIT_CAPTURES );
		code.append( langEl->tokenDef->reCaptureVect.length() );
	}

	/* Set the local frame and compile the reduce block. */
	block->compile( this, code );

	code.append( IN_PCR_RET );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

void Compiler::compilePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;

	/* Set up compilation context. */
	compileContext = CompilePreEof;
	curFunction = 0;
	revertOn = true;
	curBlock = block;
	block->frameId = nextFrameId++;

	addInput( block->localFrame );
	addThis( block->localFrame, block->context );

	CodeVect &code = block->codeWV;

	/* Set the local frame and compile the reduce block. */
	block->compile( this, code );

	code.append( IN_PCR_RET );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

int Compiler::arg0Offset()
{
	globalObjectDef->referenceField( this, arg0 );
	return arg0->offset;
}

int Compiler::argvOffset()
{
	globalObjectDef->referenceField( this, argv );
	return argv->offset;
}

int Compiler::stdsOffset()
{
	globalObjectDef->referenceField( this, stds );
	return stds->offset;
}

void Compiler::compileRootBlock( )
{
	CodeBlock *block = rootCodeBlock;

	/* The root block never needs to be reverted. */

	/* Set up the compile context. No locals are needed for the root code
	 * block, but we need an empty local frame for the compile. */
	compileContext = CompileRoot;
	curFunction = 0;
	revertOn = false;
	curBlock = 0;

	/* The block needs a frame id. */
	block->frameId = nextFrameId++;

	/* The root block is not reverted. */
	CodeVect &code = block->codeWC;

	code.append( IN_FN );
	code.append( FN_LOAD_ARG0 );
	code.appendHalf( arg0Offset() );

	code.append( IN_FN );
	code.append( FN_LOAD_ARGV );
	code.appendHalf( argvOffset() );

	code.append( IN_FN );
	code.append( FN_INIT_STDS );
	code.appendHalf( stdsOffset() );

	block->compile( this, code );

	/* A return ends the top-level code. */
	for ( LongVect::Iter rj = returnJumps; rj.lte(); rj++ ) {
		long distance = code.length() - *rj - 3;
		code.setHalf( *rj+1, distance );
	}
	returnJumps.empty();

	code.append( IN_FN );
	code.append( FN_STOP );

	/* Make the local trees descriptor. */
	findLocals( rootLocalFrame, block );
}

void ObjectField::initField()
{
	switch ( type ) {
		case UserLocalType:
		case LhsElType:
		case ParamValType:
		case RedRhsType:
			inGetR      =  IN_GET_LOCAL_R;
			inGetWC     =  IN_GET_LOCAL_WC;
			inSetWC     =  IN_SET_LOCAL_WC;
			inGetValR   =  IN_GET_LOCAL_VAL_R;
			inGetValWC  =  IN_GET_LOCAL_VAL_R;
			inGetValWV  =  IN_GET_LOCAL_VAL_R;
			inSetValWC  =  IN_SET_LOCAL_VAL_WC;
			break;

		case ParamRefType:
			inGetR  =  IN_GET_LOCAL_REF_R;
			inGetWC =  IN_GET_LOCAL_REF_WC;
			inSetWC =  IN_SET_LOCAL_REF_WC;
			break;

		case UserFieldType:
			inGetR  =  IN_GET_FIELD_TREE_R;
			inGetWC =  IN_GET_FIELD_TREE_WC;
			inGetWV =  IN_GET_FIELD_TREE_WV;
			inSetWC =  IN_SET_FIELD_TREE_WC;
			inSetWV =  IN_SET_FIELD_TREE_WV;

			//inGetValR;
			inGetValR  = IN_GET_FIELD_VAL_R;
			//inGetValWC = IN_GET_FIELD_VAL_WC;
			//inGetValWV;
			inSetValWC = IN_SET_FIELD_VAL_WC;
			inSetValWV = IN_SET_FIELD_VAL_WV;
			break;

		case GenericElementType:
		case GenericDependentType:
		case StructFieldType:
			inGetR  =  IN_GET_STRUCT_R;
			inGetWC =  IN_GET_STRUCT_WC;
			inGetWV =  IN_GET_STRUCT_WV;
			inSetWC =  IN_SET_STRUCT_WC;
			inSetWV =  IN_SET_STRUCT_WV;
			inGetValR  = IN_GET_STRUCT_VAL_R;
			inGetValWC = IN_GET_STRUCT_VAL_R;
			inGetValWV = IN_GET_STRUCT_VAL_R;
			inSetValWC = IN_SET_STRUCT_VAL_WC;
			inSetValWV = IN_SET_STRUCT_VAL_WV;
			break;

		case RhsNameType:
			inGetR  =  IN_GET_RHS_VAL_R;
			inGetWC =  IN_GET_RHS_VAL_WC;
			inGetWV =  IN_GET_RHS_VAL_WV;
			inSetWC =  IN_SET_RHS_VAL_WC;
			inSetWV =  IN_SET_RHS_VAL_WV;
			break;

		/* Inbuilts have instructions intialized outside the cons, at place of
		 * call. */
		case InbuiltFieldType:
		case InbuiltObjectType:
		case InbuiltOffType:
			break;

		/* Out of date impl. */
		case LexSubstrType:
			break;
	}
}

void ObjectDef::placeField( Compiler *pd, ObjectField *field )
{
	UniqueType *fieldUT = field->typeRef->uniqueType;

	switch ( field->type ) {
		case ObjectField::LhsElType:
		case ObjectField::UserLocalType:
		case ObjectField::RedRhsType:

			/* Local frame fields. Move the running offset first since this is
			 * a negative off from the end. */
			nextOffset += sizeOfField( fieldUT );
			field->offset = -nextOffset;
			break;


		case ObjectField::GenericElementType: {

			/* Tree object frame fields. Record the position, then move the
			 * running offset. */
			field->offset = nextOffset;
			nextOffset += sizeOfField( fieldUT );

			if ( fieldUT->typeId == TYPE_MAP_PTRS ) {
				if ( field->mapKeyField != 0 )
					field->mapKeyField->offset = field->offset;
			}

			break;
		}

		case ObjectField::UserFieldType:

			/* Tree object frame fields. Record the position, then move the
			 * running offset. */
			field->offset = nextOffset;
			nextOffset += sizeOfField( fieldUT );
			break;

		case ObjectField::StructFieldType:
			field->offset = nextOffset;
			nextOffset += sizeOfField( fieldUT );
			break;

		case ObjectField::GenericDependentType:
			/* There is an object field that this type depends on. When it is
			 * placed, this one will be placed as well. Nothing to do now. */

		case ObjectField::InbuiltFieldType:
		case ObjectField::InbuiltOffType:
		case ObjectField::InbuiltObjectType:
		case ObjectField::RhsNameType:
		case ObjectField::LexSubstrType:

		case ObjectField::ParamValType:
		case ObjectField::ParamRefType:
			break;
	}
}

void Compiler::placeAllLanguageObjects()
{
	/* Init all user object fields (need consistent size). */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		ObjectDef *objDef = lel->objectDef;
		if ( objDef != 0 ) {
			/* Init all fields of the object. */
			for ( FieldList::Iter f = objDef->fieldList; f.lte(); f++ )
				objDef->placeField( this, f->value );
		}
	}
}

void Compiler::placeAllStructObjects()
{
	for ( StructElList::Iter s = structEls; s.lte(); s++ ) {
		ObjectDef *objectDef = s->structDef->objectDef;
		for ( FieldList::Iter f = objectDef->fieldList; f.lte(); f++ )
			objectDef->placeField( this, f->value );
	}
}

void Compiler::placeFrameFields( ObjectDef *localFrame )
{
	for ( FieldList::Iter f = localFrame->fieldList; f.lte(); f++ )
		localFrame->placeField( this, f->value );
}

void Compiler::placeAllFrameObjects()
{
	/* Functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ )
		placeFrameFields( f->localFrame );

	for ( FunctionList::Iter f = inHostList; f.lte(); f++ )
		placeFrameFields( f->localFrame );

	/* Reduction code. */
	for ( ProdList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->redBlock != 0 )
			placeFrameFields( prod->redBlock->localFrame );
	}

	/* Token translation code. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 ) {
			ObjectDef *localFrame = lel->transBlock->localFrame;
			if ( lel->tokenDef->reCaptureVect.length() > 0 ) {
				FieldList::Iter f = localFrame->fieldList;
				for ( int i = 0; i < lel->tokenDef->reCaptureVect.length(); i++, f++ )
					localFrame->placeField( this, f->value );
			}

			placeFrameFields( localFrame );
		}
	}

	/* Preeof blocks. */
	for ( RegionList::Iter r = regionList; r.lte(); r++ ) {
		if ( r->preEofBlock != 0 )
			placeFrameFields( r->preEofBlock->localFrame );
	}

	/* Root code. */
	placeFrameFields( rootLocalFrame );
}

void Compiler::placeUserFunction( Function *func, bool isUserIter )
{
	/* Set up the parameters. */
	long paramPos = 0, paramListSize = 0, paramOffset = 0;
	UniqueType **paramUTs = new UniqueType*[func->paramList->length()];
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++, paramPos++ ) {
		paramUTs[paramPos] = param->typeRef->uniqueType;
		paramListSize += sizeOfField( paramUTs[paramPos] );
	}

	/* Param offset is relative to one past the last item in the array of
	 * words containing the args. */
	paramOffset = 0;
	paramPos = 0;
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++, paramPos++ ) {
		/* How much space do we need to make for call overhead. */
		long frameAfterArgs = isUserIter ? IFR_AA : FR_AA;

		param->offset = frameAfterArgs + paramOffset;

		paramOffset += sizeOfField( paramUTs[paramPos] );
	}

	func->paramListSize = paramListSize;
	func->paramUTs = paramUTs;

	func->objMethod->paramUTs = paramUTs;

	/* Insert the function into the global function map. */
	UniqueType *returnUT = func->typeRef != 0 ? 
			func->typeRef->uniqueType : uniqueTypeInt;
	func->objMethod->returnUT = returnUT;

	func->objMethod->paramUTs = new UniqueType*[func->paramList->length()];
	memcpy( func->objMethod->paramUTs, paramUTs,
			sizeof(UniqueType*) * func->paramList->length() );
}

void Compiler::placeAllFunctions()
{
	for ( FunctionList::Iter f = functionList; f.lte(); f++ )
		placeUserFunction( f, f->isUserIter );

	for ( FunctionList::Iter f = inHostList; f.lte(); f++ )
		placeUserFunction( f, false );
}


void Compiler::compileUserIter( Function *func, CodeVect &code )
{
	CodeBlock *block = func->codeBlock;

	/* The iterator starts as if from a yield, with nothing to unwind. */
	code.appendHalf( 0 );

	/* Compile the block. */
	block->compile( this, code );

	/* Always yield a nil at the end. This causes iteration to stop. Yield pops
	 * a ref, which is two words: the next pointer and the kid. Its loops have
	 * all ended, so it has nothing to unwind. */
	code.append( IN_LOAD_NIL );
	code.append( IN_LOAD_NIL );
	code.append( IN_YIELD );
	code.appendHalf( 0 );
}

void Compiler::compileUserIter( Function *func )
{
	CodeBlock *block = func->codeBlock;

	/* Set up the context. */
	compileContext = CompileFunction;
	curFunction = func;
	curBlock = block;
	block->frameId = nextFrameId++;

	/* Compile for revert and commit. */
	revertOn = true;
	compileUserIter( func, block->codeWV );

	revertOn = false;
	compileUserIter( func, block->codeWC );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

/* Called for each type of function compile: revert and commit. */
void Compiler::compileFunction( Function *func, CodeVect &code )
{
	CodeBlock *block = func->codeBlock;

	/* Compile the block. */
	block->compile( this, code );

	/* Check for a return statement. */
	if ( block->stmtList->length() == 0 ||
			block->stmtList->tail->type != LangStmt::ReturnType )
	{
		/* Push the return value. */
		code.append( IN_LOAD_NIL );
		code.append( IN_SAVE_RET );
	}

	/* Compute the jump distance for the return jumps. */
	for ( LongVect::Iter rj = returnJumps; rj.lte(); rj++ ) {
		long distance = code.length() - *rj - 3;
		code.setHalf( *rj+1, distance );
	}

	/* Reset the vector of return jumps. */
	returnJumps.empty();

	/* Return cleans up the stack (including the args) and leaves the return
	 * value on the top. */
	code.append( IN_RET );
}

void Compiler::compileFunction( Function *func )
{
	CodeBlock *block = func->codeBlock;

	/* Set up the compilation context. */
	compileContext = CompileFunction;
	curFunction = func;
	curBlock = block;

	/* Assign a frame Id. */
	block->frameId = nextFrameId++;

	/* Compile once for revert. */
	revertOn = true;
	compileFunction( func, block->codeWV );

	/* Compile once for commit. */
	revertOn = false;
	compileFunction( func, block->codeWC );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

void Compiler::removeNonUnparsableRepls()
{
	for ( ConsList::Iter repl = replList; repl.lte(); ) {
		Constructor *maybeDel = repl++;
		if ( !maybeDel->parse )
			replList.detach( maybeDel );
	}
}

void Compiler::compileByteCode()
{
	/* Compile functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ ) {
		if ( f->isUserIter )
			compileUserIter( f );
		else
			compileFunction( f );
	}

	/* Compile the reduction code. */
	for ( ProdList::Iter prod = prodList; prod.lte(); prod++ ) {
		makeProdCopies( prod );
		if ( prod->redBlock != 0 )
			compileReductionCode( prod );
	}

	/* Compile the token translation code. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 )
			compileTranslateBlock( lel );
	}

	/* Compile preeof blocks. */
	for ( RegionList::Iter r = regionList; r.lte(); r++ ) {
		if ( r->preEofBlock != 0 )
			compilePreEof( r );
	}

	/* Compile the init code */
	compileRootBlock( );
	removeNonUnparsableRepls();

	/* A call can come before the function it calls is compiled, so copies
	 * are checked once all the code is. */
	checkRefCopies();

	/* What a restartable parser runs is known once all the code is. */
	checkRestartable();

	/* So is what a function calls. */
	checkLentCalls();
}

/* A function names the context, or yields, if it does so itself, or calls a
 * function that does. A function's yield goes to the loop over the iterator
 * that called it, so an iterator it calls doesn't yield for it. */
static void closeOverCalls( FunctionList &functionList, bool CodeBlock::*flag,
		bool throughIters )
{
	bool grew = true;
	while ( grew ) {
		grew = false;
		for ( FunctionList::Iter func = functionList; func.lte(); func++ ) {
			CodeBlock *block = func->codeBlock;
			if ( block->*flag )
				continue;
			for ( Vector<Function*>::Iter c = block->calls; c.lte(); c++ ) {
				if ( (*c)->codeBlock != 0 && ( throughIters || !(*c)->isUserIter ) &&
						(*c)->codeBlock->*flag )
				{
					block->*flag = true;
					grew = true;
					break;
				}
			}
		}
	}
}

/* A call made while the running context is lent, of a function or an
 * iterator that names the context, or calls one that does, or of a function
 * that can yield to a loop's body, which could use what is lent. A call given
 * an argument that the running context reaches, of a function that names the
 * context. */
void Compiler::checkLentCalls()
{
	closeOverCalls( functionList, &CodeBlock::namesContext, true );
	closeOverCalls( functionList, &CodeBlock::yields, false );

	Vector<CodeBlock*> blocks;
	for ( FunctionList::Iter func = functionList; func.lte(); func++ )
		blocks.append( func->codeBlock );
	for ( ProdList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->redBlock != 0 )
			blocks.append( prod->redBlock );
	}
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 )
			blocks.append( lel->transBlock );
	}
	for ( RegionList::Iter region = regionList; region.lte(); region++ ) {
		if ( region->preEofBlock != 0 )
			blocks.append( region->preEofBlock );
	}

	for ( int b = 0; b < blocks.length(); b++ ) {
		for ( int c = 0; c < blocks[b]->lentCalls.length(); c++ ) {
			CodeBlock::LentCall &call = blocks[b]->lentCalls[c];
			CodeBlock *callee = call.func->codeBlock;
			if ( callee == 0 )
				continue;

			bool names = call.running && callee->namesContext;
			bool yields = !call.func->isUserIter && callee->yields;
			if ( names || yields ) {
				std::ostream &out = error( call.loc ) << "cannot call " <<
						call.func->name << " while " << call.what << " is lent (";
				ownPlace( out, call.lentLoc );
				out << "): " << call.func->name;
				if ( names )
					out << " uses the context's variables" << endp;
				else
					out << " can yield to a loop's body, which could use it" << endp;
			}
		}

		for ( int c = 0; c < blocks[b]->contextArgs.length(); c++ ) {
			CodeBlock::ContextArg &arg = blocks[b]->contextArgs[c];
			CodeBlock *callee = arg.func->codeBlock;
			if ( callee != 0 && callee->namesContext ) {
				error( arg.loc ) << "cannot give " << arg.arg << " to " << arg.func->name <<
						": " << arg.func->name << " uses the context's variables, "
						"which reach the same object" << endp;
			}
		}
	}
}

void Compiler::refFlow( bool *from, bool *to )
{
	RefFlow flow;
	flow.from = from;
	flow.to = to;
	refFlows.append( flow );
}

/* A copy given to a ref parameter, or walked by a loop, is read-only: a
 * write through it would be lost. Carry each write back along the flows, to
 * the parameters and loop variables it goes through, then refuse each copy
 * given to a parameter, or walked by a loop variable, that can be written
 * through. This is for the function or the loop, not for each path through
 * it. */
void Compiler::checkRefCopies()
{
	bool changed = true;
	while ( changed ) {
		changed = false;
		for ( Vector<RefFlow>::Iter flow = refFlows; flow.lte(); flow++ ) {
			if ( *flow->to && !*flow->from ) {
				*flow->from = true;
				changed = true;
			}
		}
	}

	for ( Vector<RefCopy>::Iter copy = refCopies; copy.lte(); copy++ ) {
		if ( copy->loopVar != 0 && copy->loopVar->refWrite ) {
			error(copy->loc) << "the loop can write through " << copy->loopVar->name <<
					", so it must be over a local, or an attribute or child of one" << endp;
		}
		if ( copy->param != 0 && copy->param->refWrite ) {
			error(copy->loc) << copy->func->name << " can write through ref parameter " <<
					copy->param->name << ", so arg " << copy->argPos + 1 <<
					" must be a local, or an attribute or child of one" << endp;
		}
	}
}

/* The types and the code blocks that a walk reaches, each queued once: what
 * restartable parsers run, or what parses given a context run. */
struct RestartReach
{
	BstSet<LangEl*> typeSet;
	BstSet<RegionSet*> regionSets;
	BstSet<CodeBlock*> blockSet;
	Vector<LangEl*> types;
	Vector<CodeBlock*> blocks;

	void type( LangEl *langEl )
	{
		if ( langEl != 0 && typeSet.insert( langEl ) )
			types.append( langEl );
	}

	void block( CodeBlock *block )
	{
		if ( block != 0 && blockSet.insert( block ) )
			blocks.append( block );
	}

	/* A token's lex block scans for all its tokens and ignores at once, and
	 * runs the action of whichever it matches. Its preeof block is kept with
	 * its tokenIgnore region. */
	void lexBlock( LelList &langEls, RegionSet *regionSet )
	{
		if ( regionSet == 0 || !regionSets.insert( regionSet ) )
			return;

		block( regionSet->tokenIgnore->preEofBlock );
		for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
			if ( lel->tokenDef != 0 && lel->tokenDef->regionSet == regionSet )
				type( lel );
		}
	}

	/* What the types and blocks queued run, and what that runs, until
	 * nothing more is queued. */
	void walk( LelList &langEls )
	{
		long nextType = 0, nextBlock = 0;
		while ( nextType < types.length() || nextBlock < blocks.length() ) {
			while ( nextType < types.length() ) {
				LangEl *lel = types[nextType++];

				for ( LelProdList::Iter prod = lel->prodList; prod.lte(); prod++ ) {
					block( prod->redBlock );
					for ( ProdElList::Iter pel = *prod->prodElList; pel.lte(); pel++ )
						type( pel->langEl );
				}

				block( lel->transBlock );
				if ( lel->tokenDef != 0 )
					lexBlock( langEls, lel->tokenDef->regionSet );
			}

			while ( nextBlock < blocks.length() ) {
				CodeBlock *codeBlock = blocks[nextBlock++];

				for ( Vector<Function*>::Iter func = codeBlock->calls; func.lte(); func++ )
					block( (*func)->codeBlock );
				for ( Vector<LangEl*>::Iter lel = codeBlock->parses; lel.lte(); lel++ )
					type( *lel );
			}
		}
	}
};

/* The first use of a global in a block that its rule refuses, as an action,
 * or in func, if it is a function's or an iterator's. What a restartable
 * parser runs can't change a global, or take a value out of one. What any
 * other parse given a context runs can't take one out, or put a reference
 * value in. */
static void reportGlobalUses( CodeBlock *block, Function *func,
		RestartReach &restart, RestartReach &context )
{
	if ( block == 0 )
		return;

	bool restartable = restart.blockSet.find( block ) != 0;
	if ( !restartable && context.blockSet.find( block ) == 0 )
		return;

	for ( Vector<GlobalUse>::Iter use = block->globalUses; use.lte(); use++ ) {
		if ( !restartable && use->kind == GlobalUse::Change && !use->putsRef )
			continue;

		std::ostream &out = error(use->loc) << "cannot ";
		if ( use->kind == GlobalUse::Change )
			out << "change " << use->global << ", a global, in ";
		else if ( use->what.length() == 0 )
			out << "take " << use->global << ", a global, out in ";
		else
			out << "take " << use->what << " out of " << use->global << ", a global, in ";

		const char *parse = restartable ? "a restartable parser" :
				"a parse given a context";
		if ( func != 0 )
			out << func->name << ", which an action of " << parse << " runs";
		else
			out << "an action of " << parse;
		out << ": keep the data in a context" << endp;
	}
}

/* A restartable parser is one the program can run its own code in while the
 * parser can still back up over its actions: one made with new parser, which
 * the program sends to, or a parse with an item after the first that runs
 * other code, and a parse that an action of one starts. Backing up assumes
 * the data is as the actions left it, so in what such a parser runs, refuse
 * each change to what a global holds, and each value taken out of a global
 * that could be changed through it. What it runs: the reduction actions of
 * what it parses, the token actions and preeof blocks of the lex blocks it
 * scans with, the functions and iterators these call, and what the parses
 * they start run. A parse given a context, restartable or not, is a parse of
 * a type defined in a context. In what it runs, refuse each value taken out
 * of a global, and each change that puts a reference value into one: either
 * would let the global and the context object hold one value, which the
 * program could change through the global while a parser lent the object
 * can back up. See doc/colm/design/global-change-design.md. */
void Compiler::checkRestartable()
{
	RestartReach restart;
	for ( LangElSet::Iter lel = restartTypes; lel.lte(); lel++ )
		restart.type( *lel );
	restart.walk( langEls );

	/* A type defined in a context is parsed only given an object of it
	 * (checkContextGiven()). */
	RestartReach context;
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->contextIn != 0 )
			context.type( lel );
	}
	context.walk( langEls );

	/* In the order the blocks were compiled, so the error is always the
	 * same one. */
	for ( FunctionList::Iter func = functionList; func.lte(); func++ )
		reportGlobalUses( func->codeBlock, func, restart, context );
	for ( ProdList::Iter prod = prodList; prod.lte(); prod++ )
		reportGlobalUses( prod->redBlock, 0, restart, context );
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ )
		reportGlobalUses( lel->transBlock, 0, restart, context );
	for ( RegionList::Iter region = regionList; region.lte(); region++ )
		reportGlobalUses( region->preEofBlock, 0, restart, context );
}

/*
 * The check that a program can't build a cycle of strong references between
 * structs. See doc/colm/design/struct-cycle-check-design.md.
 */

struct CycleNode;

/* A strong reference that a thing of one type can hold to a thing of another.
 * A field, an attribute and a make_token argument are places, where an error
 * can be. The rest are what the runtime holds for a list, a map, a parser or
 * an input. */
struct CycleEdge
{
	enum Kind
	{
		Field, Attr, MakeToken, ListValue, MapKey, MapValue,
		ParserContext, ParserInput, ParserTrees, InputTrees
	};

	CycleEdge( Kind kind, CycleNode *from, CycleNode *to )
	:
		kind(kind), from(from), to(to),
		structEl(0), langEl(0), field(0)
	{}

	Kind kind;
	CycleNode *from;
	CycleNode *to;

	/* A field's struct, an attribute's def or token, the field or the
	 * attribute, and where a place is. */
	StructEl *structEl;
	LangEl *langEl;
	ObjectField *field;
	InputLoc loc;
};

/* A type that can hold a struct: a struct, a generic, or all the tree types
 * as one. */
struct CycleNode
{
	CycleNode( GenericType *generic )
	:
		generic(generic), index(-1), lowLink(-1), onStack(false), comp(-1)
	{}

	GenericType *generic;

	/* The edges out, in the order the design lists them. */
	Vector<CycleEdge*> edges;

	/* Tarjan's algorithm, and the component it puts the node in. */
	long index;
	long lowLink;
	bool onStack;
	long comp;
};

struct CycleGraph
{
	CycleGraph();
	~CycleGraph();

	CycleNode *treeNode;
	BstMap<StructEl*, CycleNode*> structNodes;
	BstMap<GenericType*, CycleNode*> genericNodes;

	/* Every node, in the order it was made, and every edge. */
	Vector<CycleNode*> nodes;
	Vector<CycleEdge*> edges;

	/* The fields, attributes and make_token arguments, in the order an
	 * error's place is chosen. */
	Vector<CycleEdge*> places;

	Vector<CycleNode*> stack;
	long nextIndex;
	long nextComp;

	CycleNode *newNode( GenericType *generic );
	CycleNode *structNode( StructEl *structEl );
	CycleNode *genericNode( GenericType *generic );
	CycleNode *typeNode( UniqueType *ut );
	CycleEdge *addEdge( CycleEdge::Kind kind, CycleNode *from, CycleNode *to );
	CycleEdge *addPlace( CycleEdge::Kind kind, CycleNode *from, CycleNode *to,
			const InputLoc &loc );
	void strongConnect( CycleNode *v );
	void pathBack( CycleEdge *anchor, Vector<CycleEdge*> &path );
};

CycleGraph::CycleGraph()
:
	nextIndex(0), nextComp(0)
{
	treeNode = newNode( 0 );
}

CycleGraph::~CycleGraph()
{
	for ( Vector<CycleEdge*>::Iter e = edges; e.lte(); e++ )
		delete *e;
	for ( Vector<CycleNode*>::Iter n = nodes; n.lte(); n++ )
		delete *n;
}

CycleNode *CycleGraph::newNode( GenericType *generic )
{
	CycleNode *node = new CycleNode( generic );
	nodes.append( node );
	return node;
}

CycleNode *CycleGraph::structNode( StructEl *structEl )
{
	BstMapEl<StructEl*, CycleNode*> *el = structNodes.find( structEl );
	if ( el == 0 )
		el = structNodes.insert( structEl, newNode( 0 ) );
	return el->value;
}

CycleNode *CycleGraph::genericNode( GenericType *generic )
{
	BstMapEl<GenericType*, CycleNode*> *el = genericNodes.find( generic );
	if ( el == 0 )
		el = genericNodes.insert( generic, newNode( generic ) );
	return el->value;
}

/* The node that a place of type ut holds strongly, or 0. Every tree type is
 * the tree node, since an any can be put into a place of any tree type. A
 * weak reference, an int, a bool and a ref hold nothing. */
CycleNode *CycleGraph::typeNode( UniqueType *ut )
{
	switch ( ut->typeId ) {
		case TYPE_STRUCT:
			return structNode( ut->structEl );
		case TYPE_GENERIC:
			return genericNode( ut->generic );
		case TYPE_TREE:
			return treeNode;
		default:
			return 0;
	}
}

CycleEdge *CycleGraph::addEdge( CycleEdge::Kind kind, CycleNode *from, CycleNode *to )
{
	if ( to == 0 )
		return 0;

	CycleEdge *edge = new CycleEdge( kind, from, to );
	edges.append( edge );
	from->edges.append( edge );
	return edge;
}

CycleEdge *CycleGraph::addPlace( CycleEdge::Kind kind, CycleNode *from,
		CycleNode *to, const InputLoc &loc )
{
	CycleEdge *edge = addEdge( kind, from, to );
	if ( edge != 0 ) {
		edge->loc = loc;
		places.append( edge );
	}
	return edge;
}

/* Tarjan's algorithm, from v. */
void CycleGraph::strongConnect( CycleNode *v )
{
	v->index = v->lowLink = nextIndex++;
	stack.append( v );
	v->onStack = true;

	for ( Vector<CycleEdge*>::Iter e = v->edges; e.lte(); e++ ) {
		CycleNode *w = (*e)->to;
		if ( w->index < 0 ) {
			strongConnect( w );
			if ( w->lowLink < v->lowLink )
				v->lowLink = w->lowLink;
		}
		else if ( w->onStack && w->index < v->lowLink ) {
			v->lowLink = w->index;
		}
	}

	/* The root of a component: the nodes above it on the stack are in it. */
	if ( v->lowLink == v->index ) {
		CycleNode *w;
		do {
			w = stack[stack.length() - 1];
			stack.remove( stack.length() - 1 );
			w->onStack = false;
			w->comp = nextComp;
		}
		while ( w != v );
		nextComp += 1;
	}
}

/* The shortest path inside the anchor's component, from the type it holds
 * back to its owner, found breadth first, each node's edges tried in order.
 * An edge to its own node has none. */
void CycleGraph::pathBack( CycleEdge *anchor, Vector<CycleEdge*> &path )
{
	if ( anchor->to == anchor->from )
		return;

	/* The edge that first reached each node. */
	BstMap<CycleNode*, CycleEdge*> reachedBy;
	Vector<CycleNode*> queue;
	queue.append( anchor->to );
	for ( long q = 0; q < queue.length() && reachedBy.find( anchor->from ) == 0; q++ ) {
		for ( Vector<CycleEdge*>::Iter e = queue[q]->edges; e.lte(); e++ ) {
			CycleNode *w = (*e)->to;
			if ( w->comp == anchor->to->comp && w != anchor->to &&
					reachedBy.insert( w, *e ) != 0 )
				queue.append( w );
		}
	}

	for ( CycleNode *v = anchor->from; v != anchor->to; ) {
		CycleEdge *edge = reachedBy.find( v )->value;
		path.prepend( edge );
		v = edge->from;
	}
}

/* How an error names an edge. */
static String cycleStep( CycleEdge *edge )
{
	switch ( edge->kind ) {
		case CycleEdge::Field:
			return edge->structEl->name + "." + edge->field->name;
		case CycleEdge::Attr:
			return edge->langEl->name + "." + edge->field->name;
		case CycleEdge::MakeToken:
			return "a make_token argument";
		case CycleEdge::ListValue:
			return "a list's value";
		case CycleEdge::MapKey:
			return "a map's key";
		case CycleEdge::MapValue:
			return "a map's value";
		case CycleEdge::ParserContext:
			return "a parser's context";
		case CycleEdge::ParserInput:
			return "a parser's input";
		case CycleEdge::ParserTrees:
			return "a parser's trees";
		case CycleEdge::InputTrees:
			return "an input's trees";
	}
	return String();
}

/* A program that compiles can't build a cycle of strong references
 * (requirement 14 of doc/colm/design/struct-lifetime-requirements.md). Refuse
 * each struct type that can hold itself through strong references alone. A
 * node is a type that can hold a struct, and an edge says a thing of one
 * type can hold one of another strongly. A component of the graph with a
 * field, an attribute or a make_token argument inside it is a cycle, given
 * one error at the first such place. A context's field that holds a context
 * is an edge but no place, so contexts that hold contexts alone make none.
 * See doc/colm/design/struct-cycle-check-design.md. */
void Compiler::checkStrongCycles()
{
	CycleGraph graph;

	/* The fields of each struct and context. Nothing can hold the globals. */
	for ( StructElList::Iter sel = structEls; sel.lte(); sel++ ) {
		if ( sel == globalSel )
			continue;

		CycleNode *from = graph.structNode( sel );
		ObjectDef *objectDef = sel->structDef->objectDef;
		for ( FieldList::Iter f = objectDef->fieldList; f.lte(); f++ ) {
			ObjectField *field = f->value;
			if ( field->type != ObjectField::StructFieldType )
				continue;

			UniqueType *ut = field->typeRef->resolveType( this );
			CycleNode *to = graph.typeNode( ut );

			/* A field that holds a context object, which only a context's
			 * can, is no place, though a path can pass it. Nothing moves an
			 * object out of a field, and a local put into a place reached
			 * through it is refused, since noteMove() comes before the
			 * target is used. So such fields alone make chains of objects. */
			bool context = ut->typeId == TYPE_STRUCT &&
					ut->structEl->structDef->isContext;
			CycleEdge *edge = context ?
					graph.addEdge( CycleEdge::Field, from, to ) :
					graph.addPlace( CycleEdge::Field, from, to, field->loc );
			if ( edge != 0 ) {
				edge->structEl = sel;
				edge->field = field;
			}
		}
	}

	/* The attributes of each def and token. A terminal wrapper comes after
	 * its def and shares its object, so each object is walked once. */
	BstSet<ObjectDef*> objectDefs;
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->objectDef == 0 || !objectDefs.insert( lel->objectDef ) )
			continue;

		for ( FieldList::Iter f = lel->objectDef->fieldList; f.lte(); f++ ) {
			ObjectField *field = f->value;
			if ( field->type != ObjectField::UserFieldType )
				continue;

			CycleNode *to = graph.typeNode( field->typeRef->resolveType( this ) );
			if ( to == graph.treeNode )
				continue;

			CycleEdge *edge = graph.addPlace( CycleEdge::Attr,
					graph.treeNode, to, field->loc );
			if ( edge != 0 ) {
				edge->langEl = lel;
				edge->field = field;
			}
		}
	}

	/* What make_token puts into a token's attributes, whatever their types,
	 * in the order the calls were compiled. */
	for ( Vector<MakeTokenArg>::Iter arg = makeTokenArgs; arg.lte(); arg++ ) {
		graph.addPlace( CycleEdge::MakeToken, graph.treeNode,
				graph.typeNode( arg->ut ), arg->loc );
	}

	/* What a list, a map or a parser holds. An edge can make a generic node,
	 * as for a list<list<node>>, so the walk goes on over the nodes it adds. */
	for ( long n = 0; n < graph.nodes.length(); n++ ) {
		CycleNode *from = graph.nodes[n];
		GenericType *generic = from->generic;
		if ( generic == 0 )
			continue;

		switch ( generic->typeId ) {
			case GEN_LIST:
				graph.addEdge( CycleEdge::ListValue, from,
						graph.typeNode( generic->valueUt ) );
				break;
			case GEN_MAP:
				graph.addEdge( CycleEdge::MapKey, from,
						graph.typeNode( generic->keyUt ) );
				graph.addEdge( CycleEdge::MapValue, from,
						graph.typeNode( generic->valueUt ) );
				break;
			case GEN_PARSER: {
				StructDef *context = generic->elUt->langEl->contextIn;
				if ( context != 0 ) {
					graph.addEdge( CycleEdge::ParserContext, from,
							graph.structNode( context->structEl ) );
				}
				graph.addEdge( CycleEdge::ParserInput, from,
						graph.structNode( inputSel ) );
				graph.addEdge( CycleEdge::ParserTrees, from, graph.treeNode );
				break;
			}
		}
	}

	/* An input holds the trees sent to it. A stream holds no tree or
	 * struct. */
	graph.addEdge( CycleEdge::InputTrees, graph.structNode( inputSel ), graph.treeNode );

	for ( long n = 0; n < graph.nodes.length(); n++ ) {
		if ( graph.nodes[n]->index < 0 )
			graph.strongConnect( graph.nodes[n] );
	}

	/* Each component with a place inside it is a cycle, reported at the first
	 * such place. No place runs from the tree node to itself, so the tree
	 * node alone is never one. */
	BstSet<long> reported;
	for ( Vector<CycleEdge*>::Iter p = graph.places; p.lte(); p++ ) {
		CycleEdge *anchor = *p;
		if ( anchor->from->comp != anchor->to->comp ||
				!reported.insert( anchor->from->comp ) )
			continue;

		Vector<CycleEdge*> path;
		graph.pathBack( anchor, path );

		std::ostream &out = error( anchor->loc ) <<
				"strong references make a cycle: " << cycleStep( anchor );
		for ( Vector<CycleEdge*>::Iter e = path; e.lte(); e++ )
			out << ", " << cycleStep( *e );
		out << "; declare a field or attribute in it weak" << endl;
	}

	if ( reported.length() > 0 )
		exit( 1 );
}
