#include <stdlib.h>
#include <stdio.h>
#include <time.h>



void
row_sums(
  double * sums,
  const double ** matrix,
  size_t nrs,
  size_t ncs
)
{
for (size_t ix = 0; ix < nrs; ++ix) {
        double sum = 0.0;
    	const double *row = matrix[ix];
        // Unroll the loop to process four elements at a time
        size_t jx = 0;
        for (; jx + 7 < ncs; jx += 8) {
            sum += row[jx] + row[jx + 1] + row[jx + 2] + row[jx + 3] + row[jx + 4] + row[jx + 5] + row[jx + 6] +
		    row[jx + 7];
        }

        // Handle any remaining elements
        for (; jx < ncs; ++jx) {
            sum += row[jx];
        }

        sums[ix] = sum;
    }
}


int main(
		int argv,
		char *argc[]
	)
{
	
	int size = 1000;
	double **matrix = (double**) malloc(sizeof(double*) * size);
	double *entries = (double*) malloc(sizeof(double) * size * size);

	
	double *row_sum = (double*) malloc(sizeof(double) * size);

	srand(time(NULL));
	int iterations = 5000;

	for (int it = 0; it < iterations; ++it) {
			
		
		for (size_t ix = 0, jx = 0; ix < size; ++ix, jx += size) {
			matrix[ix] = entries + jx;
			for (int jix = 0; jix < size; ++jix) {
				matrix[ix][jix] = 10 * ix + jix;
			}
		}



		
		row_sums(row_sum, (const double**) matrix, size, size);

		
		
	}
	int random_index = rand() % size;
	printf("%f", row_sum[random_index]);
	

	free(matrix);
	free(entries);
	
	free(row_sum);
	return 0;

	

}
