#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

void sighandler(int);

int main(int argc, char **argv) {
	int pFile, oflags;
	struct sigaction action, oa;
	int ii, count = 0;

	// Opens to device file
	pFile = open("/dev/fasync_example", O_RDWR);
	if (pFile < 0) {
		fprintf (stderr, "fasync_example module isn't loaded\n");
		return 1;
	}

	// Setup signal handler
	memset(&action, 0, sizeof(action));
	action.sa_handler = sighandler;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);
	sigaction(SIGIO, &action, NULL);
	fcntl(pFile, F_SETOWN, getpid());
	oflags = fcntl(pFile, F_GETFL);
	fcntl(pFile, F_SETFL, oflags | FASYNC);

	// Write to file.
	write(pFile, "Nothing", 8);
	
	// Waits.
	printf("Sleep!\n");
	pause();

	printf("Closing out!\n");
	// Closes.
	close(pFile);
	return 0;
	
}

// SIGIO handler
void sighandler(int signo)
{
	printf("Awaken!\n");
}
