#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <stdint.h>

typedef struct {
  int val;
  char pad[60]; // cacheline - sizeof(int)
} int_padded;


typedef struct {
  const float **v;
  float **w;
  int ib;
  int istep;
  int sz;
  int tx;
  mtx_t *mtx;
  cnd_t *cnd;
  int_padded *status;
} thrd_info_t;

typedef struct {
  const float **v;
  float **w;
  int sz;
  int nthrds;
  mtx_t *mtx;
  cnd_t *cnd;
  int_padded *status;
  FILE *attractors_file;
  FILE *convergence_file;
  int d;
} thrd_info_check_t;


uint8_t ** attractors;
uint8_t ** convergences;


int colors[10][3] = {
    {249, 65, 68},    // From top row
    {144, 190, 109},  // From bottom row
    {243, 114, 44},   // From top row
    {67, 170, 139},   // From bottom row
    {248, 150, 30},   // From top row
    {77, 144, 142},   // From bottom row
    {249, 132, 74},   // From top row
    {87, 117, 144},   // From bottom row
    {249, 199, 79},   // From top row
    {39, 125, 161}    // From bottom row
};



int
main_thrd(
    void *args
    )
{
  const thrd_info_t *thrd_info = (thrd_info_t*) args;
  const float **v = thrd_info->v;
  float **w = thrd_info->w;
  const int ib = thrd_info->ib;
  const int istep = thrd_info->istep;
  const int sz = thrd_info->sz;
  const int tx = thrd_info->tx;
  mtx_t *mtx = thrd_info->mtx;
  cnd_t *cnd = thrd_info->cnd;
  int_padded *status = thrd_info->status;

  uint8_t * attractor;
  uint8_t * convergence;

  printf("Thread %d: Starting computation at index %d with step %d and size %d\n", tx, ib, istep, sz);
  for ( int ix = ib; ix < sz; ix += istep ) {
    const float *vix = v[ix];
    // We allocate the rows of the result before computing, and free them in another thread.
    float *wix = (float*) malloc(sz*sizeof(float));

    attractor = (uint8_t*) malloc(sz*sizeof(uint8_t));
    convergence = (uint8_t*) malloc(sz*sizeof(uint8_t));


    for ( size_t cx = 0; cx < sz; ++cx ) {
      attractor[cx] = 0;
      convergence[cx] = 0;
    }

    for ( int jx = 0; jx < sz; ++jx )
      wix[jx] = sqrtf(vix[jx]);


    mtx_lock(mtx);
    attractors[ix] = attractor;
    convergences[ix] = convergence;
    w[ix] = wix;
    status[tx].val = ix + istep;
    mtx_unlock(mtx);
    cnd_signal(cnd);

    // In order to illustrate thrd_sleep and to force more synchronization
    // points, we sleep after each line for one micro seconds.
    thrd_sleep(&(struct timespec){.tv_sec=0, .tv_nsec=1000}, NULL);
  }

  return 0;
}

int
main_thrd_check(
    void *args
    )
{
  const thrd_info_check_t *thrd_info = (thrd_info_check_t*) args;
  const float **v = thrd_info->v;
  float **w = thrd_info->w;
  const int sz = thrd_info->sz;
  const int nthrds = thrd_info->nthrds;
  mtx_t *mtx = thrd_info->mtx;
  cnd_t *cnd = thrd_info->cnd;
  int_padded *status = thrd_info->status;

  const float eps = 1e-1;

  // We do not increment ix in this loop, but in the inner one.
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

      // Instead of employing a conditional variable, we could also invoke
      // thrd_yield or thrd_sleep in order to yield to other threads or grant a
      // specified time to the computation threads.
    }

    fprintf(stderr, "checking from index %i until %i\n", ix, ibnd);

    // We do not initialize ix in this loop, but in the outer one.
    for ( ; ix < ibnd; ++ix ) {
      // We only check the last element in w, since we want to illustrate the
      // situation where the check thread completes fast than the computaton
      // threads.
      int jx = sz-1;
      float diff = v[ix][jx] - w[ix][jx] * w[ix][jx];
      if ( diff < -eps || diff > eps ) {
        fprintf(stderr, "incorrect compuation at %i %i: %f %f %f\n",
            ix, jx, diff, v[ix][jx], w[ix][jx]);
        // This exists the whole program, including all other threads.
        exit(1);
      }

      // We free the component of w, since it will never be used again.
      free(w[ix]);
    }
  }

  return 0;
}

int
main_thrd_write(
    void *args
    )
{
  const thrd_info_check_t *thrd_info = (thrd_info_check_t*) args;
  const float **v = thrd_info->v;
  float **w = thrd_info->w;
  const int sz = thrd_info->sz;
  const int nthrds = thrd_info->nthrds;
  mtx_t *mtx = thrd_info->mtx;
  cnd_t *cnd = thrd_info->cnd;
  int_padded *status = thrd_info->status;
  FILE *attractors_file = thrd_info->attractors_file;
  FILE *convergence_file = thrd_info->convergence_file;
  const int d = thrd_info->d;

  // We do not increment ix in this loop, but in the inner one.
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

    fprintf(stderr, "writing from index %i until %i\n", ix, ibnd);

    // We do not initialize ix in this loop, but in the outer one.
    for ( ; ix < ibnd; ++ix ) {
      for (int jx = 0; jx < sz; ++jx) {
        // Write attractor data
        int color_index = (int)(fabs(sin(w[ix][jx])) * 9); // Ensure index is within bounds
        
        uint8_t attractor_data[3] = {
            (uint8_t)(colors[color_index][0]),
            (uint8_t)(colors[color_index][1]),
            (uint8_t)(colors[color_index][2])
        };
        printf("attractor_data: %d %d %d\n", attractor_data[0], attractor_data[1], attractor_data[2]);

        char buffer[50]; // Buffer to hold the formatted string
        sprintf(buffer, "%d %d %d ", attractor_data[0], attractor_data[1], attractor_data[2]);
        fwrite(buffer, sizeof(char), strlen(buffer), attractors_file);

        // Write convergence data
        int conv = (int)(255 * (1 - exp(-fabs(v[ix][jx] - w[ix][jx]))));
        uint8_t convergence_data[3] = { (uint8_t)conv, (uint8_t)conv, (uint8_t)conv };
        printf("convergence_data: %d %d %d ", convergence_data[0], convergence_data[1], convergence_data[2]);

        
        sprintf(buffer, "%d %d %d ", convergence_data[0], convergence_data[1], convergence_data[2]);
        fwrite(buffer, sizeof(char), strlen(buffer), convergence_file);
      }
      fputc('\n', attractors_file);
      fputc('\n', convergence_file);
    }
  }

  return 0;
}




// Global variables for number of threads and size of the output picture (rows and columns)
int nthrds;
int sz;




int main(int argc, char *argv[]) {
    int d;  // Exponent of x in the polynomial x^d - 1

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
	
  float **v = (float**) malloc(sz*sizeof(float*));
  float **w = (float**) malloc(sz*sizeof(float*));
  float *ventries = (float*) malloc(sz*sz*sizeof(float));
  // The entries of w will be allocated in the computation threads are freed in
  // the check thread.

  FILE *attractors_file = fopen("newton_attractors_xd.ppm", "w");
  FILE *convergence_file = fopen("newton_convergence_xd.ppm", "w");

  fprintf(attractors_file, "P3\n%d %d\n%d\n", sz, sz, 255);
  fprintf(convergence_file, "P3\n%d %d\n255\n", sz, sz);

  for ( int ix = 0, jx = 0; ix < sz; ++ix, jx += sz )
    v[ix] = ventries + jx;

  for ( int ix = 0; ix < sz*sz; ++ix )
    ventries[ix] = ix;

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
    thrds_info[tx].v = (const float**) v;
    thrds_info[tx].w = w;
    thrds_info[tx].ib = tx;
    thrds_info[tx].istep = nthrds;
    thrds_info[tx].sz = sz;
    thrds_info[tx].tx = tx;
    thrds_info[tx].mtx = &mtx;
    thrds_info[tx].cnd = &cnd;
    thrds_info[tx].status = status;
    status[tx].val = -1;

    int r = thrd_create(thrds+tx, main_thrd, (void*) (thrds_info+tx));
    if ( r != thrd_success ) {
      fprintf(stderr, "failed to create thread\n");
      exit(1);
    }
    thrd_detach(thrds[tx]);
  }

  {
    thrd_info_check.v = (const float**) v;
    thrd_info_check.w = w;
    thrd_info_check.sz = sz;
    thrd_info_check.nthrds = nthrds;
    thrd_info_check.mtx = &mtx;
    thrd_info_check.cnd = &cnd;
    thrd_info_check.status = status;
    thrd_info_check.attractors_file = attractors_file;
    thrd_info_check.convergence_file = convergence_file;
    thrd_info_check.d = d;

    int r = thrd_create(&thrd_check, main_thrd_check, (void*) (&thrd_info_check));
    if ( r != thrd_success ) {
      fprintf(stderr, "failed to create check thread\n");
      exit(1);
    }
  }

  {
    int r = thrd_create(&thrd_write, main_thrd_write, (void*) (&thrd_info_check));
    if ( r != thrd_success ) {
      fprintf(stderr, "failed to create write thread\n");
      exit(1);
    }
  }

  {
    int r;
    thrd_join(thrd_check, &r);
    thrd_join(thrd_write, &r);
  }

  free(ventries);
  free(v);
  free(w);

  fclose(attractors_file);
  fclose(convergence_file);

  mtx_destroy(&mtx);
  cnd_destroy(&cnd);

  return 0;

}

