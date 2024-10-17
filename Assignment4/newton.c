#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <stdint.h>
#include <complex.h>

#define MAX_DEGREE 9
#define MAX_ITERATIONS 128


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

complex float roots[MAX_DEGREE][MAX_DEGREE];
int d;  

typedef struct {
  int val;
  char pad[60]; // cacheline - sizeof(int)
} int_padded;


typedef struct {
  int ib;
  int istep;
  int sz;
  int tx;
  mtx_t *mtx;
  cnd_t *cnd;
  int_padded *status;
  complex float (*handle_degree)(complex float);
} thrd_info_t;

typedef struct {
  int sz;
  int nthrds;
  mtx_t *mtx;
  cnd_t *cnd;
  int_padded *status;
  FILE *attractors_file;
  FILE *convergence_file;
  int d;
} thrd_info_check_t;


complex float degree_1(complex float x) {

  return 1.0;
}

complex float degree_2(complex float x) {

  return (x + (1 / x)) / 2.0;
}

complex float degree_3(complex float x) {
  
  return (2 * x + (1 / (x * x))) / 3.0;
}

complex float degree_4(complex float x) {

  return (3 * x + (1 / (x * x * x))) / 4.0;
}

complex float degree_5(complex float x) {

  return (4 * x + (1 / (x * x * x * x))) / 5.0;
}

complex float degree_6(complex float x) {

  return (5 * x + (1 / (x * x * x * x * x))) / 6.0;
}

complex float degree_7(complex float x) {

  return (6 * x + (1 / (x * x * x * x * x * x))) / 7.0;
}

complex float degree_8(complex float x) {

  return (7 * x + (1 / (x * x * x * x * x * x * x))) / 8.0; 
}

complex float degree_9(complex float x) {

  return (8 * x + (1 / (x * x * x * x * x * x * x * x))) / 9.0;
}

complex float degree_10(complex float x) {

  return (9 * x + (1 / (x * x * x * x * x * x * x * x * x))) / 10.0;
}






uint8_t ** attractors;
uint8_t ** convergences;



const char *colors[11] = {
    "0 51 102 ",      // Dark Blue
    "0 102 102 ",     // Teal
    "0 153 153 ",     // Dark Cyan
    "0 102 51 ",      // Dark Green
    "0 153 51 ",      // Medium Green
    "0 204 102 ",     // Green
    "51 204 153 ",    // Medium Sea Green
    "51 153 102 ",    // Olive Green
    "102 204 153 ",   // Light Sea Green
    "102 153 153 ",    // Light Slate Gray
    "255 0 0 ",        // Red
};

const char *grayscale[128];

void initialize_grayscale() {
    for (int i = 0; i < 128; i++) {
        char *color = malloc(12); // Allocate memory for each color string
        if (color) {
            sprintf(color, "%d %d %d ", i * 2, i * 2, i * 2);
            grayscale[i] = color;
        }
    }
}







int
main_thrd(
    void *args
    )
{
  const thrd_info_t *thrd_info = (thrd_info_t*) args;
  const int ib = thrd_info->ib;
  const int istep = thrd_info->istep;
  const int sz = thrd_info->sz;
  const int tx = thrd_info->tx;
  mtx_t *mtx = thrd_info->mtx;
  cnd_t *cnd = thrd_info->cnd;
  int_padded *status = thrd_info->status;
  complex float (*handle_degree)(complex float) = thrd_info->handle_degree;
  for ( int ix = ib; ix < sz; ix += istep ) {

   uint8_t *attractor = (uint8_t*) malloc(sz*sizeof(uint8_t));
   uint8_t *convergence = (uint8_t*) malloc(sz*sizeof(uint8_t));

    // Calculate the imaginary part of the complex plane, take the negative because we want to start at the top left corner
    float imaginary_part = (-2.0f + (4.0f * (float)ix) / ((float)sz - 1)) * -1;
    
    for ( size_t cx = 0; cx < sz; ++cx ) {
      attractor[cx] = 10; // last index in color array
      convergence[cx] = 127;
      float real_part = -2.0f + (4.0f * (float)cx) / ((float)sz - 1);
      complex float z = real_part + imaginary_part * I;

      for (int conv = 0; conv < MAX_ITERATIONS; ++conv) {
        if (fabs(creal(z)) > 1e10 || fabs(cimag(z)) > 1e10) {
          attractor[cx] = 10; // last index in color array
          convergence[cx] = conv;
          break;
        }

        
        float norm_squared = creal(z) * creal(z) + cimag(z) * cimag(z);

        // check for lower bound of the absolute value of x
        // if (norm_squared < 2e-3) {

        //   attractor[cx] = d + 1;
        //   convergence[cx] = conv;
        //   break;
        // }

        if (norm_squared <= (1 + 2e-3) && norm_squared >= (1 - 2e-3)) {
          
        
         
          for (int root_index = 0; root_index < d; root_index++) {
            complex float root = roots[d-1][root_index];
            if (cabs(z - root) < 1e-3) {
              attractor[cx] = root_index;
              
              convergence[cx] = conv;
              break;
            }
          }

          if (attractor[cx] != 10) {
            break;
          }

          
          
        }

        z = handle_degree(z);

      }

      
    }

    mtx_lock(mtx);
    attractors[ix] = attractor;
    convergences[ix] = convergence;
    status[tx].val = ix + istep;

    mtx_unlock(mtx);
    cnd_signal(cnd);

  }

  return 0;
}


int
main_thrd_write(
    void *args
    )
{
  const thrd_info_check_t *thrd_info = (thrd_info_check_t*) args;
  const int sz = thrd_info->sz;
  const int nthrds = thrd_info->nthrds;
  mtx_t *mtx = thrd_info->mtx;
  cnd_t *cnd = thrd_info->cnd;
  int_padded *status = thrd_info->status;
  FILE *attractors_file = thrd_info->attractors_file;
  FILE *convergence_file = thrd_info->convergence_file;
  const int d = thrd_info->d;

  // We do not increment ix in this loop, but in the inner one.
  const float eps = 1e-1;
  for ( int ix = 0, ibnd; ix < sz; ) {
    
    // If no new lines are available, we wait.
    for ( mtx_lock(mtx); ; ) {
      // We extract the minimum of all status variables.
      ibnd = sz;
      for ( int tx = 0; tx < nthrds; ++tx )
        if ( ibnd > status[tx].val )
          ibnd = status[tx].val;

      if ( ibnd <= ix )
        // Until here the mutex protects status updates, so that if further
        // updates are pending in blocked threads, there will be a subsequent
        // signal.
        cnd_wait(cnd,mtx);
      else {
        mtx_unlock(mtx);
        break;
      }
    }


    // We do not initialize ix in this loop, but in the outer one.
    for ( ; ix < ibnd; ++ix ) {
      for (int jx = 0; jx < sz; ++jx) {
        // Write attractor data
        uint8_t color_index = attractors[ix][jx];
        

        fwrite(colors[color_index], sizeof(char), strlen(colors[color_index]), attractors_file);

        // Write convergence data
        int conv = convergences[ix][jx];

        fwrite(grayscale[conv], sizeof(char), strlen(grayscale[conv]), convergence_file);
        
        
      }
      fputc('\n', attractors_file);
      fputc('\n', convergence_file);
      free(attractors[ix]);
      free(convergences[ix]);
    }
  }

  return 0;
}




void initialize_roots() {
    for (int dimension = 1; dimension <= MAX_DEGREE; dimension++) {
        printf("Roots for x^%d - 1:\n", dimension);
        for (int k = 0; k < dimension; k++) {
            roots[dimension-1][k] = (float) cos(2 * M_PI * k / dimension) + I * sin(2 * M_PI * k / dimension);
            // Print each root
            printf("Root %d: %.6f + %.6fi\n", k, creal(roots[dimension-1][k]), cimag(roots[dimension-1][k]));
        }
        printf("\n");
    }
}

// Global variables for number of threads and size of the output picture (rows and columns)
int nthrds;
int sz;


int main(int argc, char *argv[]) {
    

    // Iterate through command-line arguments
    for (int ix = 1; ix < argc; ix++) {
        // Check for the "-t" argument (threads)
        if (strncmp(argv[ix], "-t", 2) == 0) {
            nthrds = atoi(argv[ix] + 2); // Convert to integer and store in global variable
        }
        // Check for the "-l" argument (picture size)
        else if (strncmp(argv[ix], "-l", 2) == 0) {
            sz = atoi(argv[ix] + 2); // Convert to integer and store in global variable
        }
        // If it's not an option, assume it's the final argument (exponent d)
        else {
            d = atoi(argv[ix]); // Convert the last argument to an integer for exponent
        }
    }

    // Print out the parsed values
    printf("Number of threads: %d\n", nthrds);
    printf("Picture size: %d x %d\n", sz, sz);
    printf("Polynomial exponent: %d (for x^%d - 1)\n", d, d);

    // Here would be the code to start the Newton iteration using the provided arguments
	complex float (*handle_degree)(complex float);

  switch (d) {
    case 1:
         handle_degree = degree_1;
         break;
    case 2:
         handle_degree = degree_2;
         break;
    case 3:
         handle_degree = degree_3;
         break;
    case 4:
         handle_degree = degree_4;
         break;
    case 5:
         handle_degree = degree_5;
         break;
    case 6:
         handle_degree = degree_6;
         break;
    case 7:
         handle_degree = degree_7;
         break;
    case 8:
         handle_degree = degree_8;
         break;
    case 9:
         handle_degree = degree_9;
         break;
    case 10:
         handle_degree = degree_10;
         break;
    default:
        handle_degree = NULL;
  }

  // The entries of w will be allocated in the computation threads are freed in
  // the check thread.
  initialize_roots();
  initialize_grayscale();
  char filename_attractors[30];
  char filename_convergence[30];
  sprintf(filename_attractors, "newton_attractors_x%d.ppm", d);
  sprintf(filename_convergence, "newton_convergence_x%d.ppm", d);
  FILE *attractors_file = fopen(filename_attractors, "w");
  FILE *convergence_file = fopen(filename_convergence, "w");

  fprintf(attractors_file, "P3\n%d %d\n%d\n", sz, sz, 255);
  fprintf(convergence_file, "P3\n%d %d\n255\n", sz, sz);


  thrd_t thrds[nthrds];
  thrd_info_t thrds_info[nthrds];

  thrd_t thrd_check;
  thrd_t thrd_write;
  thrd_info_check_t thrd_info_check;
  
  mtx_t mtx;
  mtx_init(&mtx, mtx_plain);

  cnd_t cnd;
  cnd_init(&cnd);

  int_padded status[nthrds];

  attractors = (uint8_t**) malloc(sz*sizeof(uint8_t*));
  convergences = (uint8_t**) malloc(sz*sizeof(uint8_t*));

  for ( int tx = 0; tx < nthrds; ++tx ) {
    thrds_info[tx].ib = tx;
    thrds_info[tx].istep = nthrds;
    thrds_info[tx].sz = sz;
    thrds_info[tx].tx = tx;
    thrds_info[tx].mtx = &mtx;
    thrds_info[tx].cnd = &cnd;
    thrds_info[tx].status = status;
    thrds_info[tx].handle_degree = handle_degree;
    status[tx].val = 0;

    int r = thrd_create(thrds+tx, main_thrd, (void*) (thrds_info+tx));
    if ( r != thrd_success ) {
      fprintf(stderr, "failed to create thread\n");
      exit(1);
    }
    thrd_detach(thrds[tx]);
  }


  {

    thrd_info_check.sz = sz;
    thrd_info_check.nthrds = nthrds;
    thrd_info_check.mtx = &mtx;
    thrd_info_check.cnd = &cnd;
    thrd_info_check.status = status;
    thrd_info_check.attractors_file = attractors_file;
    thrd_info_check.convergence_file = convergence_file;
    thrd_info_check.d = d;
    int r = thrd_create(&thrd_write, main_thrd_write, (void*) (&thrd_info_check));
    if ( r != thrd_success ) {
      fprintf(stderr, "failed to create write thread\n");
      exit(1);
    }
  }

  {
    int r;
    thrd_join(thrd_write, &r);
  }


  free(attractors);
  free(convergences);

  fclose(attractors_file);
  fclose(convergence_file);

  mtx_destroy(&mtx);
  cnd_destroy(&cnd);

  return 0;

}

