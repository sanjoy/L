#include <stdio.h>
#include <stdlib.h>

#include "l-repl.h"

int
main (int argc, char **argv)
{
	if (argc < 2) {
		l_start_repl (NULL);
	} else {
		l_start_repl (argv [1]);
	}
	return 0;
}
