#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************
  Usage:
    ./ktimer [flag] [number] [message]

        -l (list)
        -s (set)
		-m (max)

  Examples:
        ./ktimer -l
				List all active timers
        ./ktimer -s 5 "ThisIsAMessage"
				Set a timer for 5 seconds with message "ThisIsAMessage"
				If a timer with the same message exists, it will be updated
		./ktimer -m 5
				Set the max number of active timers to 5

******************************************************/

int main (int argc, char *argv[]) {
	char buffer[256];
	
	if (argc < 2)
	{
		// print man page?
		return -1;
	}
	
	FILE * pFile;
	pFile = fopen("/dev/mytimer", "r+");
	
	if (pFile == NULL) {
		fputs("mytimer module isn't loaded\n", stderr);
		return -1;
	}

	/* Check for valid command line arguments */
	if (argc == 2 && strcmp(argv[1], "-l") == 0) {
		// list timers to stdout
		while (fgets(buffer, sizeof(buffer) - 1, pFile) != NULL) {
			printf("%s\n", buffer);
		}

	} else if (argc == 4 && strcmp(argv[1], "-s") == 0) {
		// reg new timer based on parameters
		fprintf(pFile, "s %s %s", argv[2], argv[3]);
			
	} else if (argc == 3 && strcmp(argv[1], "-m") == 0) {
		// change max active timer count to given [COUNT]
		fprintf(pFile, "m %s", argv[2]);
	} else {
		// return manual page?
	}
	
	fclose(pFile);
	return 0;
}

