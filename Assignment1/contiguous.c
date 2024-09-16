#include <stdio.h>
#include <stdlib.h>

int main(
	int argc,
	char *argv[]
	)
{

int size = 10;

int *asentries = (int*) (malloc(sizeof(int) * size * size));
int **as = (int**) (malloc(sizeof(int*) * size));

for (size_t ix = 0, jx = 0; ix < size; ix++, jx += size) {
    as[ix] = asentries + jx;
}

return 0;
}
