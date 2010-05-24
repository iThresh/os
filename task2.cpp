#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define INPUT_WAIT_A 1
#define INPUT_WAIT_B 2
#define INPUT_WAIT_C 3
#define INPUT_WAIT_PIPE 4
#define INPUT_A 5
#define INPUT_B 6
#define INPUT_C 7
#define INPUT_ERROR 8
 
int pipe_Aout_Bin[2];
int pipe_Aerr_Cin[2];
int A_pid, B_pid, C_pid;

void close_pipes() {
	close( pipe_Aout_Bin[ 0 ] );
	close( pipe_Aout_Bin[ 1 ] );
	close( pipe_Aerr_Cin[ 0 ] );
	close( pipe_Aerr_Cin[ 1 ] );
}
void close_pipe( int apipe[2] ) {
	close( apipe[ 0 ] );
	close( apipe[ 1 ] );
}

int main() {
	while (1) {
		char input[ 256 ];
		char A[ 256 ], B[ 256 ], C[ 256 ];
		int lenA = 0, lenB = 0, lenC = 0;
		memset(input, 0, 256*sizeof(char));
		memset(A, 0, 256*sizeof(char));
		memset(B, 0, 256*sizeof(char));
		memset(C, 0, 256*sizeof(char));
		printf("> ");
		fgets(input, 256, stdin);
		
		if(( strncmp(input, "quit", 4) == 0 )&&( strlen(input) == 5 ))
			exit( 0 );

		int len = strlen( input ) - 1;
		input[len] = 0;

		int state = INPUT_WAIT_A;

		/* PARSE INPUT */
		for (int i = 0; i < len; i++) {
			switch (state) {
				case INPUT_WAIT_A:
					if( input[ i ] != ' ' ) {
						A[ lenA++ ] = input[ i ];
						state = INPUT_A;
					}
					break;
					
				case INPUT_A:
					if( input[ i ] == ' ' ) {
						state = INPUT_WAIT_PIPE;
					} else if ( input[ i ] == '|' ) {
						state = INPUT_WAIT_B;
					} else {
						A[ lenA++ ] = input[ i ];
					}
					break;
					
				case INPUT_WAIT_PIPE:
					if( input[ i ] == '|' ) {
						state = INPUT_WAIT_B;
					} else if ( input[ i ] != ' ' ) {
						state = INPUT_ERROR;
					}
					break;
					
				case INPUT_WAIT_B:
					if( input[ i ] != ' ' ) {
						B[ lenB++ ] = input[ i ];
						state = INPUT_B;
					}
					break;
					
				case INPUT_B:
					if( input[ i ] == ' ' ) {
						state = INPUT_WAIT_C;
					} else {
						B[ lenB++ ] = input[ i ];
					}
					break;
					
				case INPUT_WAIT_C:
					if( input[ i ] != ' ' ) {
						C[ lenC++ ] = input[ i ];
						state = INPUT_C;
					}
					break;
					
				case INPUT_C:
					C[ lenC++ ] = input[ i ];
					break;
			}
		}
		if( state != INPUT_C ) {
			printf("Usage: A | B C\n\tA.out goes to B.in\n\tA.err goes to C.in\nTo quit say quit.\n");
			continue;
		}

		printf("A: %s, B: %s, C: %s\n", A, B, C);
		/* CREATE PIPES */
		if( pipe(pipe_Aout_Bin) ) {
			fprintf(stderr, "pipe: failed\n");
			continue;
		}
		if( pipe(pipe_Aerr_Cin) ) {
			fprintf(stderr, "pipe: failed\n");
			close_pipe( pipe_Aout_Bin );
			continue;
		}
		
		/* CREATE PROGRAM A */
		A_pid = fork();

		if( A_pid < 0 ) { /* ERROR */
			fprintf(stderr, "fork: failed\n");
			close_pipes();
			continue;
		}
		else if ( A_pid == 0 ) { /* CHILD */	 
			if( dup2(pipe_Aout_Bin[ 1 ], STDOUT_FILENO) == -1 ) {
				fprintf(stderr, "dup2: A.out to B.in failed in program A\n");
				exit( 0 );
			}
			if (dup2(pipe_Aerr_Cin[ 1 ], STDERR_FILENO) == -1) {
				fprintf(stderr, "dup2: A.err to C.in failed in program A\n");
				exit( 0 );
			}
			close( pipe_Aout_Bin[ 0 ] );
			close( pipe_Aerr_Cin[ 0 ] );
			execlp( A, A, NULL);
			exit( 0 );
		}
		else { /* PARENT */ 

		}
		
		/* CREATE PROGRAM B */
		B_pid = fork();

		if( B_pid < 0 ) { /* ERROR */
			fprintf(stderr, "fork: failed\n");
			close_pipes();
			continue;
		}
		else if ( B_pid == 0 ) { /* CHILD */	 
			if( dup2(pipe_Aout_Bin[ 0 ], STDIN_FILENO) == -1 ) {
				fprintf(stderr, "dup2: A.out to B.in failed in program B\n");
				exit( 0 );
			}
			close( pipe_Aout_Bin[ 1 ] );
			close( pipe_Aerr_Cin[ 0 ] );
			close( pipe_Aerr_Cin[ 1 ] );
			execlp( B, B, NULL);
			exit( 0 );
		}
		else { /* PARENT */ 

		}
		
		/* CREATE PROGRAM C */
		C_pid = fork();

		if( C_pid < 0 ) { /* ERROR */
			fprintf(stderr, "fork: failed\n");
			close_pipes();
			continue;
		}
		else if ( C_pid == 0 ) { /* CHILD */	 
			if( dup2(pipe_Aerr_Cin[ 0 ], STDIN_FILENO) == -1 ) {
				fprintf(stderr, "dup2: A.err to C.in failed in program C\n");
				exit( 0 );
			}
			close( pipe_Aout_Bin[ 0 ] );
			close( pipe_Aout_Bin[ 1 ] );
			close( pipe_Aerr_Cin[ 1 ] );
			execlp( A, A, NULL);
			exit( 0 );
		}
		else { /* PARENT */ 

		}
		
		close_pipes();
		int status;
		wait(&status);
		wait(&status);
		wait(&status);
	}
}
