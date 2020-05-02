#define _GNU_SOURCE

#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


static void* (*original_malloc)(size_t ) = NULL;

// Find the address of a symbol starting from our preloaded SO (malicious.so)
void *findSymbolAddress(char *symbol)
{
	void *address = dlsym(RTLD_NEXT, symbol);
	if (address == NULL) {
		fprintf(stderr, "[-] (%s) Cannot find symbol address: %s\n", __FUNCTION__, symbol);
		exit(0x10);
	}

	return address;
}


// Do not use printf() or fprintf(stdout, ...) inside this wrapper as they call malloc internally
// and this may lead to infinite loop (out wrapper calling our wrapper) 
void *malloc(size_t size)
{
	if ( original_malloc == NULL)
		original_malloc = (void *)findSymbolAddress("malloc");

	{
		// Do your stuff here
		fprintf(stderr, "\n\t[-x-x-x-] Hijacked libc's malloc(%ld)\n\n", size);
	}

	// Calling original malloc and return to caller
	void *alloc_addr = NULL; 
	alloc_addr = original_malloc(size);
	return alloc_addr;
}

