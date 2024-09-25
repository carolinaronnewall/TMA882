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
        for (; jx + 3 < ncs; jx += 4) {
            sum += row[jx] + row[jx + 1] + row[jx + 2] + row[jx + 3];
        }

        // Handle any remaining elements
        for (; jx < ncs; ++jx) {
            sum += row[jx];
        }

        sums[ix] = sum;
    }
}

void
col_sums(
  double * sums,
  const double ** matrix,
  size_t nrs,
  size_t ncs
  )
{
// Initialize the sums array to zero
    for (size_t jx = 0; jx < ncs; ++jx) {
        sums[jx] = 0.0;
    }

    // Cache-friendly summation
    for (size_t ix = 0; ix < nrs; ++ix) {
        const double *row = matrix[ix];  // Cache the row pointer

        // Unroll the inner loop for better performance
        size_t jx = 0;
        size_t unroll_limit = ncs - (ncs % 4);  // Handle chunks of 4
        for (; jx < unroll_limit; jx += 4) {
            sums[jx]   += row[jx];
            sums[jx+1] += row[jx+1];
            sums[jx+2] += row[jx+2];
            sums[jx+3] += row[jx+3];
        }

        // Handle remaining elements if ncs is not a multiple of 4
        for (; jx < ncs; ++jx) {
            sums[jx] += row[jx];
        }
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

	double *col_sum = (double*) malloc(sizeof(double) * size);
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

		
		col_sums(col_sum, (const double**) matrix, size, size);
		
	}
	int random_index = rand() % size;
	printf("%f", col_sum[random_index]);
	

	free(matrix);
	free(entries);
	
	free(col_sum);
	free(row_sum);
	return 0;

	

}
