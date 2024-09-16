#include <stdlib.h>
#include <stdio.h>



int
main(
        int argc,
        char *argv[])

{
int * as;
// as = (int*)malloc(10*sizeof(int));
int sum = 0;

for ( int ix = 0; ix < 10; ++ix )
  as[ix] = ix;

for ( int ix = 0; ix < 10; ++ix )
  sum += as[ix];

printf("%d\n", sum);

free(as);
return 0;
}
