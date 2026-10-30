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

#ifndef _PERL_CODEGEN_H
#define _PERL_CODEGEN_H

#include "gendata.h"

class PerlCodeGen : public CodeGenData
{
public:
	explicit PerlCodeGen( ostream &out ) : CodeGenData(out) { }

	void finishRagelDef();
	void writeData();
	void writeInit();
	void writeExec();
	void writeExports();
	void writeStart();
	void writeFirstFinal();
	void writeError();

private:
	void action( RedAction *a );
	void condition( GenAction *a );
	void expression( GenInlineList *list );
	std::string variable( GenInlineList *list, const char *fallback );
	int firstFinal();
	int errorState();
};

#endif
