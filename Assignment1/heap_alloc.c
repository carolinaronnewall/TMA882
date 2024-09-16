#include <stdio.h>
#include <stdlib.h>
int main(
	int argc,
	char *argv[]
	)
{


int size = 100;


int *as = (int*) (malloc(sizeof(int) * size));

for (size_t ix = 0; ix < size; ix++) {
	as[ix] = 0;
}


printf("%d\n", as[0]);

free(as);

}
