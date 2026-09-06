/*
 * Copyright 2026 Adrian Thurston <thurston@colm.net>
 */

#include "harness.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Pipes must be close-on-exec: workers fork concurrently, and a child that
 * inherits another job's pipe end keeps that pipe from ever reaching EOF. */
static bool makePipe( int fds[2] )
{
#ifdef __linux__
	return pipe2( fds, O_CLOEXEC ) == 0;
#else
	if ( pipe( fds ) != 0 )
		return false;
	fcntl( fds[0], F_SETFD, FD_CLOEXEC );
	fcntl( fds[1], F_SETFD, FD_CLOEXEC );
	return true;
#endif
}

static void closeFd( int &fd )
{
	if ( fd >= 0 ) {
		close( fd );
		fd = -1;
	}
}

/* Move data between the child and our buffers until every pipe is done.
 * Descriptors are closed as they finish and set to -1 in the caller: closing
 * a number twice would close a descriptor another thread has since opened. */
static void pump( int &inFd, const std::string *stdinData,
		int &outFd, std::string *stdoutBuf, int &errFd, std::string *stderrBuf )
{
	size_t written = 0;
	char buf[65536];

	while ( inFd >= 0 || outFd >= 0 || errFd >= 0 ) {
		struct pollfd pfd[3];
		int n = 0;
		int inIdx = -1, outIdx = -1, errIdx = -1;

		if ( inFd >= 0 ) {
			inIdx = n;
			pfd[n].fd = inFd;
			pfd[n].events = POLLOUT;
			n++;
		}
		if ( outFd >= 0 ) {
			outIdx = n;
			pfd[n].fd = outFd;
			pfd[n].events = POLLIN;
			n++;
		}
		if ( errFd >= 0 ) {
			errIdx = n;
			pfd[n].fd = errFd;
			pfd[n].events = POLLIN;
			n++;
		}

		if ( poll( pfd, n, -1 ) < 0 ) {
			if ( errno == EINTR )
				continue;
			break;
		}

		if ( inIdx >= 0 && pfd[inIdx].revents != 0 ) {
			if ( pfd[inIdx].revents & POLLOUT ) {
				size_t left = stdinData->size() - written;
				if ( left > sizeof(buf) )
					left = sizeof(buf);
				ssize_t w = write( inFd, stdinData->data() + written, left );
				if ( w < 0 ) {
					if ( errno != EINTR && errno != EAGAIN )
						closeFd( inFd );
				}
				else {
					written += w;
					if ( written >= stdinData->size() )
						closeFd( inFd );
				}
			}
			else {
				/* Child closed its end. */
				closeFd( inFd );
			}
		}

		if ( outIdx >= 0 && pfd[outIdx].revents != 0 ) {
			ssize_t r = read( outFd, buf, sizeof(buf) );
			if ( r > 0 )
				stdoutBuf->append( buf, r );
			else if ( r == 0 || ( errno != EINTR && errno != EAGAIN ) )
				closeFd( outFd );
		}

		if ( errIdx >= 0 && pfd[errIdx].revents != 0 ) {
			ssize_t r = read( errFd, buf, sizeof(buf) );
			if ( r > 0 )
				stderrBuf->append( buf, r );
			else if ( r == 0 || ( errno != EINTR && errno != EAGAIN ) )
				closeFd( errFd );
		}
	}
}

bool runProcess( const Words &argv, const std::string &cwd,
		const std::string *stdinData, const std::string &stdinFile,
		std::string *stdoutBuf, std::string *stderrBuf,
		int &exitCode, std::string &errMsg )
{
	exitCode = -1;

	if ( argv.empty() ) {
		errMsg = "empty command";
		return false;
	}

	std::vector<char*> args;
	for ( Words::const_iterator w = argv.begin(); w != argv.end(); w++ )
		args.push_back( const_cast<char*>( w->c_str() ) );
	args.push_back( 0 );

	/* Reports errno from between fork and exec. */
	int errPipe[2] = { -1, -1 };
	int inPipe[2] = { -1, -1 };
	int outPipe[2] = { -1, -1 };
	int stderrPipe[2] = { -1, -1 };

	if ( !makePipe( errPipe ) ||
			( stdinData != 0 && !makePipe( inPipe ) ) ||
			( stdoutBuf != 0 && !makePipe( outPipe ) ) ||
			( stderrBuf != 0 && !makePipe( stderrPipe ) ) )
	{
		errMsg = std::string( "pipe: " ) + strerror( errno );
		closeFd( errPipe[0] ); closeFd( errPipe[1] );
		closeFd( inPipe[0] ); closeFd( inPipe[1] );
		closeFd( outPipe[0] ); closeFd( outPipe[1] );
		closeFd( stderrPipe[0] ); closeFd( stderrPipe[1] );
		return false;
	}

	pid_t pid = fork();
	if ( pid < 0 ) {
		errMsg = std::string( "fork: " ) + strerror( errno );
		closeFd( errPipe[0] ); closeFd( errPipe[1] );
		closeFd( inPipe[0] ); closeFd( inPipe[1] );
		closeFd( outPipe[0] ); closeFd( outPipe[1] );
		closeFd( stderrPipe[0] ); closeFd( stderrPipe[1] );
		return false;
	}

	if ( pid == 0 ) {
		/* Child. Only async-signal-safe calls from here to exec. */
		int fd;

		if ( stdinData != 0 )
			fd = inPipe[0];
		else if ( !stdinFile.empty() )
			fd = open( stdinFile.c_str(), O_RDONLY );
		else
			fd = open( "/dev/null", O_RDONLY );
		if ( fd < 0 )
			goto fail;
		if ( dup2( fd, 0 ) < 0 )
			goto fail;

		if ( stdoutBuf != 0 )
			fd = outPipe[1];
		else
			fd = open( "/dev/null", O_WRONLY );
		if ( fd < 0 )
			goto fail;
		if ( dup2( fd, 1 ) < 0 )
			goto fail;

		if ( stderrBuf != 0 ) {
			if ( dup2( stderrPipe[1], 2 ) < 0 )
				goto fail;
		}

		if ( !cwd.empty() && chdir( cwd.c_str() ) != 0 )
			goto fail;

		/* Children get default signal dispositions. */
		signal( SIGPIPE, SIG_DFL );

		execvp( args[0], &args[0] );

	fail: {
			int e = errno;
			ssize_t r = write( errPipe[1], &e, sizeof(e) );
			(void)r;
			_exit( 127 );
		}
	}

	/* Parent. */
	closeFd( errPipe[1] );
	closeFd( inPipe[0] );
	closeFd( outPipe[1] );
	closeFd( stderrPipe[1] );

	if ( stdinData != 0 && stdinData->empty() )
		closeFd( inPipe[1] );

	pump( inPipe[1], stdinData, outPipe[0], stdoutBuf, stderrPipe[0], stderrBuf );

	closeFd( inPipe[1] );
	closeFd( outPipe[0] );
	closeFd( stderrPipe[0] );

	int childErrno = 0;
	ssize_t r;
	do {
		r = read( errPipe[0], &childErrno, sizeof(childErrno) );
	} while ( r < 0 && errno == EINTR );
	closeFd( errPipe[0] );

	int status = 0;
	while ( waitpid( pid, &status, 0 ) < 0 ) {
		if ( errno != EINTR )
			break;
	}

	if ( r == (ssize_t)sizeof(childErrno) ) {
		errMsg = argv[0] + ": " + strerror( childErrno );
		return false;
	}

	if ( WIFEXITED( status ) )
		exitCode = WEXITSTATUS( status );
	else if ( WIFSIGNALED( status ) )
		exitCode = 128 + WTERMSIG( status );
	else
		exitCode = -1;

	return true;
}
