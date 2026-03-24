#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

/******************************************************
  Usage:
    ./ktimer [flag] [number] [message]

        -l (list)
        -s (set)
		-r (remove)
		-m (max)

  Examples:
        ./ktimer -l
				List all active timers
        ./ktimer -s 5 "ThisIsAMessage"
				Set a timer for 5 seconds with message "ThisIsAMessage"
				If a timer with the same message exists, it will be updated
		./ktimer -r
				Remove all active timers
		./ktimer -m 5
				Set the max number of active timers to 5

******************************************************/

int pFile;

void sigio_handler(int signo) {
    char buffer[256];
    ssize_t bytesRead;
	// printf("SIGIO handler\n"); // temp
    // Read expired timer message from device
    while ((bytesRead = read(pFile, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0'; 
		printf("%s", buffer);
    }
}

int main (int argc, char *argv[]) {
    char buffer[256];
	int timerExists = 0;
	
	if (argc < 2){
		return -1;
	}
	
    pFile = open("/dev/mytimer", O_RDWR);
	
	if (pFile < 0) {
		return -1;
	}
	


	/* Check for valid command line arguments */
	if ((argc == 2 || (argc == 3 && strcmp(argv[2], "&") == 0)) && strcmp(argv[1], "-l") == 0) {
		// tell kernel module to list timers
		write(pFile, "l", strlen("l"));
		// list timers to stdout
		ssize_t bytesRead;
		while ((bytesRead = read(pFile, buffer, sizeof(buffer) - 1)) > 0) {
			buffer[bytesRead] = '\0';
			printf("%s", buffer);
		}
	} else if ((argc == 4 || (argc == 5 && strcmp(argv[4], "&") == 0)) && strcmp(argv[1], "-s") == 0) {
		// reg new timer based on parameters
		char *sec = argv[2];
		char *msg = argv[3];

		write(pFile, "l", 1);
        ssize_t bytesRead;
		// check if timer already exists
        while ((bytesRead = read(pFile, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            if (strstr(buffer, msg)) {
                timerExists = 1;
            }
        }
		memset(buffer, 0, sizeof(buffer));
        if (timerExists) {
            // update existing timer
			// printf("update timer\n"); // temp
            snprintf(buffer, sizeof(buffer), "u %s %s", sec, msg);
            write(pFile, buffer, strlen(buffer));
			memset(buffer, 0, sizeof(buffer));
			while ((bytesRead = read(pFile, buffer, sizeof(buffer) - 1)) > 0) {
				// printf("read in ul\n", buffer); // temp
				buffer[bytesRead] = '\0';
				printf("%s", buffer);
			}
        } else {
            // register new timer
			// printf("new timer\n"); // temp
            snprintf(buffer, sizeof(buffer), "s %s %s", sec, msg);
            write(pFile, buffer, strlen(buffer));
			memset(buffer, 0, sizeof(buffer));
			while ((bytesRead = read(pFile, buffer, sizeof(buffer) - 1)) > 0) {
				// printf("read in ul\n", buffer); // temp
				buffer[bytesRead] = '\0';
				printf("%s", buffer);
			}
			if (bytesRead == 0) {
				// Setup signal handler
				struct sigaction action;
				memset(&action, 0, sizeof(action));
				action.sa_handler = sigio_handler;
				sigemptyset(&action.sa_mask);
				sigaction(SIGIO, &action, NULL);
				fcntl(pFile, F_SETOWN, getpid());
				int oflags = fcntl(pFile, F_GETFL);
				fcntl(pFile, F_SETFL, oflags | FASYNC);
			}
        }
		// printf("pausing\n"); // temp
		pause();
			
	} else if ((argc == 2 || (argc == 3 && strcmp(argv[2], "&") == 0)) && strcmp(argv[1], "-r") == 0) {
		// remove all timers
		write(pFile, "r", strlen("r"));
		timerExists = 0;
	} else if ((argc == 3 || (argc == 4 && strcmp(argv[3], "&") == 0)) && strcmp(argv[1], "-m") == 0) {
		ssize_t bytesRead;
		// change max active timer count to given [COUNT]
		snprintf(buffer, sizeof(buffer), "m %s", argv[2]);
		write(pFile, buffer, strlen(buffer));
		memset(buffer, 0, sizeof(buffer));
		while ((bytesRead = read(pFile, buffer, sizeof(buffer) - 1)) > 0) {
			buffer[bytesRead] = '\0';
			printf("%s", buffer);
		}
	} else {
		// return manual page?
	}
	// printf("exiting\n"); // temp
	// fclose(pFile);
    close(pFile);
	return 0;
}

