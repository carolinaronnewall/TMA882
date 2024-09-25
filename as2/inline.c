#include <stdlib.h>
#include <stdio.h>
#include <time.h>


int main(
		int argv,
		char *argc[])
{
	
		
	int size = 30000;
	double *as_re = (double*) malloc(sizeof(double) * size);
	double *bs_re = (double*) malloc(sizeof(double) * size);
	double *cs_re = (double*) malloc(sizeof(double) * size);
	
	double *as_im = (double*) malloc(sizeof(double) * size);
	double *bs_im = (double*) malloc(sizeof(double) * size);
	double *cs_im = (double*) malloc(sizeof(double) * size);
	

	srand(time(NULL));
	
	int iterations = 500000;
	for (int it = 0; it < iterations; ++it) {
		for (int ix = 0; ix < size; ++ix) {
			
			bs_re[ix] = (double) ix + 1;
			cs_re[ix] = (double) ix;
			
			bs_im[ix] = (double) ix + 5;
			cs_im[ix] = (double) ix;

		}

		for (int ix = 0; ix < size; ++ix) {
			

			as_re[ix] = bs_re[ix] * cs_re[ix] - bs_im[ix] * cs_im[ix];
			as_im[ix] = bs_re[ix] * cs_im[ix] + bs_im[ix] * cs_re[ix];
			
			
				
		}
	}

	int random_index = rand() % size;
	
	printf("%f\n", as_re[random_index]);

	free(as_re);
	free(bs_re);
	free(cs_re);
	free(as_im);
	free(bs_im);
	free(cs_im);


	return 0;



}
