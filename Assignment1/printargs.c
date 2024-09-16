#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(
    int argc,
    char *argv[]
    )

{


int a;
int b;
for (int ix = 1; ix < argc; ix++) {
        if (strncmp(argv[ix], "-a", 2) == 0) {
            // Convert the part after "-a" to an integer
            a = atoi(argv[ix] + 2); // Skip the "-a" part
        } else if (strncmp(argv[ix], "-b", 2) == 0) {
            // Convert the part after "-b" to an integer
            b = atoi(argv[ix] + 2); // Skip the "-b" part
        }	
	
    }

printf("A is %i and B is %i\n", a, b);
return 0;


}
