/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

/*
 * The runtime's own declarations, which no program or host needs. This header
 * is not installed.
 */

#ifndef _COLM_RTERROR_H
#define _COLM_RTERROR_H

struct colm_program;

/* Stops the program when a read or write on a stream fails, with the name of
 * the stream and the errno of the call that failed. WHAT is "read" or
 * "write". Defined in bytecode.c, beside runtime_error, which it stops the
 * program through. */
void colm_stream_error( struct colm_program *prg, const char *what,
		const char *name, int err );

#endif /* _COLM_RTERROR_H */
