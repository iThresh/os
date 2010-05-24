#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

void sigusr1_handler( int sig ) {
	int pid = getpid();
	printf("Process ( pid = %i, gid = %i ) caught a signal %i.\n", pid, getpgid(pid), sig);
}

void child() {
	while( true ) {
		pause();
	}
}

int main( int argc, char** argv ) {

	/* CREATE A GROUP */
	if (setpgid(0, 0) != 0) {
		fprintf(stderr, "setpgid: failed.\n");
		exit(0);
	}

	int gid = getpid();
	printf("Session ID: %i\n", getsid(gid));
	printf("Group ID: %i\n", gid);

	struct sigaction sa;
	memset( &sa, 0, sizeof( struct sigaction ) );
	sa.sa_handler = sigusr1_handler;
	sigaction( SIGUSR1, &sa, 0 );

	for (int i = 0; i < 5; i++) {
		int pid = fork();

		if( pid < 0 ) { /* ERROR */
			fprintf(stderr, "fork: failed\n");
			exit( -1 );
		}
		else if ( pid == 0 ) { /* CHILD */	 
			child();
		}
		else { /* PARENT */ 
			
		}
	}

	for(int i = 0; i < 3; i++) {
		sleep(2);
		kill(-gid, SIGUSR1);
	}
	
	kill(-gid, SIGHUP);
	return 0;
}
