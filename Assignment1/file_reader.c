#include <stdio.h>
#include <stdlib.h>


int main(
	int argc,
	char *argv[]
	)
{


    FILE *file = fopen("matrix.bin", "r");

    if (file == NULL) {
    	return -1;
    }
    int size = 10;
    int matrix[size][size];

    fread((void*) matrix, sizeof(int), size*size, file);

    fclose(file);
    for (int ix = 0; ix < size; ix++ ){
	    for (int jx = 0; jx < size; jx++)
	    {
		if (matrix[ix][jx] != ix * jx) {
		   printf("Wrong entry!!!!!!!");
		} else {
		
		printf("%i ", matrix[ix][jx]);

		}

	    }
	    printf("\n");
    }

return 0;

}
