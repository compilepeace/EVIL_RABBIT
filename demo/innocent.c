#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	char *alloc = (char *)malloc(0x100);
	alloc = "I'm feeling good\n";
	
	
	// Seems like printf() and fprintf(stdout, ...) also calls malloc() internally which resolves to 
	// our hook around original malloc() implementation.
	fprintf(stderr, "%p: %s\n", alloc, alloc);
}
