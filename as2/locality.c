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
  for ( size_t ix = 0; ix < nrs; ++ix ) {
    double sum = 0.;
    for ( size_t jx = 0; jx < ncs; ++jx )
      sum += matrix[ix][jx];
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
  for ( size_t jx = 0; jx < ncs; ++jx ) {
    double sum = 0.;
    for ( size_t ix = 0; ix < nrs; ++ix )
      sum += matrix[ix][jx];
    sums[jx] = sum;
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
