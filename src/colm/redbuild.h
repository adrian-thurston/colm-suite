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

#ifndef _COLM_FSMREDUCE_H
#define _COLM_FSMREDUCE_H

#include <iostream>

#include <avltree.h>

#include "libfsm/fsmgraph.h"

#include "compiler.h"

/* Forwards. */
struct Compiler;
struct FsmCodeGen;
struct RedFsm;

struct RedActionTable
:
	public AvlTreeEl<RedActionTable>
{
	RedActionTable( const ActionTable &key )
	:	
		key(key), 
		id(0)
	{ }

	const ActionTable &getKey() 
		{ return key; }

	ActionTable key;
	int id;
};

typedef AvlTree<RedActionTable, ActionTable, CmpActionTable> ActionTableMap;

/*
 * Builds the reduced machine from the libfsm graph. Colm never embeds
 * conditions in its scanners, so every transition in the graph is a plain
 * transition and the walk asserts as much.
 */
class RedFsmBuild
{
public:
	RedFsmBuild( Compiler *pd, FsmAp *fsm );
	RedFsm *reduceMachine( );

private:
	void appendTrans( TransListVect &outList, Key lowKey, Key highKey, TransAp *trans );
	void makeStateActions( StateAp *state );
	void makeStateList();

	void initActionList( unsigned long length );
	void newAction( int anum, char *name, int line, int col, Action *action );
	void initActionTableList( unsigned long length );
	void initStateList( unsigned long length );
	void addRegionToEntry( int regionId, int entryId );
	void addEntryPoint( int entryId, unsigned long entryState );
	void setId( int snum, int id );
	void initTransList( int snum, unsigned long length );
	void newTrans( int snum, int tnum, Key lowKey, Key highKey, 
			long targ, long act );
	void finishTransList( int snum );
	void setFinal( int snum );
	void setEofTrans( int snum, int eofTarget, int actId );
	void setStateActions( int snum, long toStateAction, 
			long fromStateAction, long eofAction );
	void setForcedErrorState();
	void closeMachine();
	Key findMaxKey();

	void makeEntryPoints();
	void makeActionList();
	void makeActionTableList();
	void reduceActionTables();
	void makeTransList( StateAp *state );
	void makeTrans( Key lowKey, Key highKey, TransDataAp *trans );
	void makeAction( Action *action );
	void makeMachine();

	Compiler *pd;
	FsmAp *fsm;
	KeyOps *keyOps;
	ActionTableMap actionTableMap;
	int nextActionTableId;

	int startState;
	int errState;

public:
	RedFsm *redFsm;

private:
	int curAction;
	int curActionTable;
	int curTrans;
	int curState;
};

#endif /* _COLM_FSMREDUCE_H */

