#include <stdio.h>
#include <stdlib.h>


int main(
	int argc,
	char * argv[]
	)
{
	

int size = 10;

FILE *file = fopen("matrix.bin", "w");

if (file == NULL) {
    printf("error opening file \n");
    return -1;

    }


int matrix[size][size];
for (int ix = 0; ix < size; ix++) {
    for (int jx = 0; jx < size; jx++) {
		matrix[ix][jx] = ix * jx;

	}

}

fwrite((void*) matrix, sizeof(int), size * size, file);
fclose(file);

return 0;

}
