/*
 * Copyright 2002 Adrian Thurston <thurston@colm.net>
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

#include <iostream>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "util.h"

#define TAB_WIDTH 10

/* Once a stop is requested the test has this long to finish its round and the
 * final verification before the default SIGALRM action kills it. */
#define GRACE_SECS 30

using namespace std;

volatile sig_atomic_t stopRequested = 0;

static void requestStop( int )
{
	stopRequested = 1;

	/* From here a second interrupt kills the process, as does the grace alarm
	 * if the test fails to exit in time. */
	signal( SIGINT, SIG_DFL );
	signal( SIGTERM, SIG_DFL );
	signal( SIGALRM, SIG_DFL );
	alarm( GRACE_SECS );
}

void processArgs( int argc, char** argv )
{
	int secs = argc > 1 ? atoi( argv[1] ) : 0;
	unsigned seed = argc > 2 ? strtoul( argv[2], 0, 10 ) :
			(unsigned) time(0) ^ ( (unsigned) getpid() << 16 );

	srandom( seed );
	srand48( seed );

	if ( secs > 0 )
		fprintf( stderr, "seed %u, running for %d seconds\n", seed, secs );
	else
		fprintf( stderr, "seed %u, running until interrupted\n", seed );

	signal( SIGINT, &requestStop );
	signal( SIGTERM, &requestStop );
	if ( secs > 0 ) {
		signal( SIGALRM, &requestStop );
		alarm( secs );
	}
}

/* Expand the tabs in a buffer a buffer. */
void expandTab( char *dst, const char *src )
{
	char *pd = dst;
	for ( ; *src != 0; src++ ) {
		if ( *src != '\t' )
			*pd++ = *src;
		else {
			int n = TAB_WIDTH - ((pd - dst)%TAB_WIDTH);
			memset( pd, ' ', n );
			pd += n;
		}
	}
	*pd = 0;
}


