#include <stdlib.h>
#include <stdio.h>
#include <time.h>


int main(
	int argc,
	char *argv[]
	)

{


	long int iterations = 1000000000;
	

	long long int sum = 0;
	double total_t;	
	clock_t start_t, end_t;
	
	start_t = clock();
   	printf("Starting of the program, start_t = %ld\n", start_t);
	for (long int ix = 1; ix < iterations + 1; ix++) {
		sum += ix;
	}


	end_t = clock();
   	printf("End of the big loop, end_t = %ld\n", end_t);


	total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
   	printf("Total time taken by CPU: %f\n", total_t  );
   	printf("The sum is: %lli \n", sum);
	return 0;
}
