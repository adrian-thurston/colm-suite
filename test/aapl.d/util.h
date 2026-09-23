#ifndef __UTIL_H
#define __UTIL_H

#include <signal.h>

/* Set when the run time given on the command line is up, or on the first
 * SIGINT or SIGTERM. The main loop of a stress test runs while it is clear,
 * then does a final verification and returns 0. */
extern volatile sig_atomic_t stopRequested;

/* Usage: prog [SECONDS [SEED]]. SECONDS bounds the run; 0 or absent runs until
 * interrupted. SEED seeds random() and drand48(), by default from the clock.
 * The seed in use is reported on stderr. */
void processArgs( int argc, char** argv );

void expandTab( char *dst, const char *src );

#endif
