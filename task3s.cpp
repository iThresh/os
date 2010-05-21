#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sched.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>

#define STACK_SIZE 1024*64
#define TABLE_X 40
#define TABLE_Y 25

#define PORT 31337

int TABLE1 [ TABLE_X*TABLE_Y ];
int TABLE2 [ TABLE_X*TABLE_Y ];
int* actual_table;
int* previous_table;

int count_neighbours ( int* table, int x, int y ){
	int sum = 0;
	
	if(( x != 0 )&&( y != 0 ))
		sum += table[(y-1)*TABLE_X + x-1];
		
	if(( x != 0 )&&( y != TABLE_Y-1 ))
		sum += table[(y+1)*TABLE_X + x-1];
		
	if(( x != TABLE_X-1 )&&( y != 0 ))
		sum += table[(y-1)*TABLE_X + x+1];
		
	if(( x != TABLE_X-1 )&&( y != TABLE_Y-1 ))
		sum += table[(y+1)*TABLE_X + x+1];
		
	if( x != 0 )
		sum += table[y*TABLE_X + x-1];
		
	if( x != TABLE_X-1 )
		sum += table[y*TABLE_X + x+1];
		
	if( y != 0 )
		sum += table[(y-1)*TABLE_X + x];
		
	if( y != TABLE_Y-1 )
		sum += table[(y+1)*TABLE_X + x];
		
	return sum;
}

void calculate_life() {
	for (int i = 0; i < TABLE_X; i++) {
		for (int j = 0; j < TABLE_Y; j++) {
			int neighbours = count_neighbours (actual_table, i, j);
			if (( neighbours < 2 )||( neighbours > 3 ))
				previous_table[j*TABLE_X + i] = 0;
			if ( neighbours == 3 )
				previous_table[j*TABLE_X + i] = 1;
			if ( neighbours == 2 )
				previous_table[j*TABLE_X + i] = actual_table[j*TABLE_X + i];	
		}
	}
	int* tmp = previous_table;
	previous_table = actual_table;
	actual_table = tmp;
}

int load_seed() {
	std::ifstream f("./life.txt");
	
	for (int i = 0; i < TABLE_Y; i++) {
		for (int j = 0; j < TABLE_X; j++) {
			char cell;
			cell = f.get();
			if (cell == '0') {
				TABLE1[i*TABLE_X + j] = 0;
				TABLE2[i*TABLE_X + j] = 0;
			} else {
				TABLE1[i*TABLE_X + j] = 1;
				TABLE2[i*TABLE_X + j] = 1;
			}
		}
		char tmp[ 256 ];
		f.getline(tmp, 256);
	}
	actual_table = TABLE1;
	previous_table = TABLE2;
	f.close();
}

void print_table (int* table) {
	for (int i = 0; i < TABLE_Y; i++) {
		for(int j = 0; j < TABLE_X; j++) {
			if (table[i*TABLE_X + j] == 0)
				printf("0 ");
			else
				printf("* ");
		}
		printf("\n");
	}
	printf("\n");
}

int thread_lifecalculator(void* argument) {
	while (true) {
		sleep (1);
		calculate_life();
	}
}

int thread_actualtable_sender(void* argument) {
	int sockfd = (int) argument;
	int n;
	for (int i = 0; i < TABLE_Y; i++) {
		for (int j = 0; j < TABLE_X; j++) {
			if (actual_table[i*TABLE_X + j] == 0)
				n = write(sockfd,"0 ",2);
			else
				n = write(sockfd,"* ",2);
				
			if ( n < 0 ) { /* SOCKET CLOSED?? */
				return( 1 );
			}
		}
		n = write(sockfd,"\n",2);
		if ( n < 0 ) { /* AGAIN, SOCKET CLOSED?? */
			return( 1 );
		}
	}
	return 0;
}

int thread_connectionhandler(void* argument) {
	int sockfd = (int) argument;
	char buf[ 256 ];	
	while (true) {
		memset(buf,0,256);
		int n = read(sockfd,buf,255);
		if ( n < 0 ) { /* SOCKET CLOSED ?? */
			fprintf(stderr, "read: no read from socket\n");
			close(sockfd);
			return( 1 );
		}
		
		/* CLOSE CONNECTION */ 
		if ( strncmp( buf, "quit", 4 ) == 0 ) {
			close(sockfd);
			return( 0 );

		/* GIVE TABLE */
		} else if ( strncmp( buf, "get", 3 ) == 0 ) {
				int pid;
				pid = fork();

				if( pid < 0 ) { /* ERROR */
					fprintf(stderr, "fork failed\n");
					return( 2 );
				}
				else if ( pid == 0 ) { /* CHILD */	 
					return thread_actualtable_sender((void*)sockfd);
				}
				else { /* PARENT */ 
					int code;
					wait(&code);
					if ( code == 1 ) {
						fprintf(stderr, "write: no write to socket\n");
						close(sockfd);
						return( 1 );
					}
				}
		} else {
			n = write(sockfd,"Please type get or quit.\n",26);
			if ( n < 0 ) { /* AGAIN, SOCKET CLOSED?? */
				fprintf(stderr, "write: no write to socket\n");
				close(sockfd);
				return( 1 );
			}
		}
	}
}

int thread_stub( void* argument ) {
	int pid;
	pid = fork();

	if( pid < 0 ) { /* ERROR */
		fprintf(stderr, "fork failed\n");
		exit( -1 );
	}
	else if ( pid == 0 ) { /* CHILD */	 

	}
	else { /* PARENT */ 

	}
}

int main()
{
	load_seed();
	
	/* CREATE A THREAD TO CALCULATE LIFE EVERY SECOND */
	
	void* stack = malloc( STACK_SIZE );
	if ( stack == 0 ) {
		fprintf(stderr, "malloc: could not allocate stack\n" );
		exit( 1 );
	}

	int pid = clone( &thread_lifecalculator, (char*) stack + STACK_SIZE,
		SIGCHLD | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_VM | CLONE_THREAD, 0 );
	if ( pid == -1 ) {
		fprintf(stderr, "clone: failed\n" );
		exit( 2 );
	}
	
	/* SERVER */
	
	struct sockaddr_in serv_addr; /* server address */
	struct sockaddr_in cli_addr; /* client address */

	/* CREATE A SOCKET, BIND AND LISTEN */
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( sockfd < 0 ) {
		fprintf(stderr, "socket: could not open socket\n");
		exit( 1 );
	}
	
	memset((void*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "bind: failed\n");
		exit( 1 );
	}
	listen(sockfd,5);
	
	/* ACCEPT CONNECTION */
	while (true) {
		//socklen_t cliaddrlen = sizeof(cli_addr);
		//int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cliaddrlen);
		int newsockfd = accept(sockfd, 0, 0);
		if ( newsockfd < 0 ) {
			fprintf (stderr, "accept: failed\n");
		}
		
		/* CREATE A THREAD TO HANDLE THE CONNECTION */
		
		void* newstack = malloc( STACK_SIZE );
		if ( newstack == 0 ) {
			fprintf(stderr, "malloc: could not allocate stack\n" );
			exit( 1 );
		}

		int pid = clone( &thread_connectionhandler, (char*) newstack + STACK_SIZE,
			SIGCHLD | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_VM | CLONE_THREAD, (void*)newsockfd);
		if ( pid == -1 ) {
			fprintf(stderr, "clone: failed\n" );
			close(newsockfd);
			exit( 2 );
		}
	}
	
	/* DEBUG */
	for (int i = 0; i < 1000; i++) {
		print_table(actual_table);
		sleep(1);
	}

	//free( stack );
	exit(0);
}
