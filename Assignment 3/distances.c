#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>

#define MAX_CELLS 3  // Maximum number of coordinates
#define MAX_DISTANCE_INDEX 3464  // Adjust based on expected maximum distance
#define CELL_FILE "cells"

// Function to parse coordinate from string
static inline void parse_coord_row(const char *ptr, int16_t *coords) {
    // ptr points to start a row of coordinates (+dd.ddd +dd.ddd +dd.ddd)
    for (int i = 0; i < 3; i++) {
        int sign = (ptr[0] == '-') ? -1 : 1;

        int16_t integer_part = (ptr[1] - '0') * 10 + (ptr[2] - '0'); // two digits
        int16_t fractional_part = (ptr[4] - '0') * 100 + (ptr[5] - '0') * 10 + (ptr[6] - '0'); // three digits

        int16_t value = integer_part * 1000 + fractional_part;
        coords[i] = value * sign;

        // Move to the next coordinate
        ptr += 8; // Move past the current coordinate and the space

}


void calculate_distances_in_chunk(int16_t *coords, int num_cells, long int **counts_array) {
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_cells - 1; ++i) {
        int tid = omp_get_thread_num();
        long int *local_counts = counts_array[tid];
        int16_t xi = coords[i * 3 + 0];
        int16_t yi = coords[i * 3 + 1];
        int16_t zi = coords[i * 3 + 2];
        for (int j = i + 1; j < num_cells; ++j) {
            int16_t dx = xi - coords[j * 3 + 0];
            int16_t dy = yi - coords[j * 3 + 1];
            int16_t dz = zi - coords[j * 3 + 2];

            int32_t dist_sq = (int32_t)dx * dx + (int32_t)dy * dy + (int32_t)dz * dz;
            double dist = sqrt((double)dist_sq);

            int16_t distance_scaled = (int16_t)(dist / 10 + 0.5); // Scale to two decimal places

            if (distance_scaled >= 0 && distance_scaled < MAX_DISTANCE_INDEX) {
                local_counts[distance_scaled]++;
            }
        }
    }
}

void calculate_distances_between_chunks(int16_t *coords, int16_t *coords2, int num_cells, int second_chunk_size, long int **counts_array) {
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_cells - 1; ++i) {
        int tid = omp_get_thread_num();
        long int *local_counts = counts_array[tid];
        int16_t xi = coords[i * 3 + 0];
        int16_t yi = coords[i * 3 + 1];
        int16_t zi = coords[i * 3 + 2];

        for (int j = 0; j < second_chunk_size; ++j) {
            int16_t xj = coords2[j * 3 + 0];
            int16_t yj = coords2[j * 3 + 1];
            int16_t zj = coords2[j * 3 + 2];

            int32_t dx = xi - xj;
            int32_t dy = yi - yj;
            int32_t dz = zi - zj;

            int32_t dist_sq = dx * dx + dy * dy + dz * dz;
            double dist = sqrt((double)dist_sq);

            int16_t distance_scaled = (int16_t)(dist / 10 + 0.5); // Scale to two decimal places

            if (distance_scaled >= 0 && distance_scaled < MAX_DISTANCE_INDEX) {
                local_counts[distance_scaled]++;
            }
        }


    }
}



// void print_chunk(FILE *fp, int bytes_to_read) {
//         long int chunk_start = ftell(fp);
//         unsigned char *buffer = malloc(bytes_to_read);
//         if (!buffer) {
//             printf("Memory allocation failed\n");
//             return;
//         }

//         size_t bytes_read = fread(buffer, 1, bytes_to_read, fp);

//         printf("Chunk start: %ld\n", chunk_start);
//         printf("Bytes read: %zu\n", bytes_read);
//         printf("Bytes to read: %d\n", bytes_to_read);

//         if (bytes_read < bytes_to_read) {
//             if (feof(fp)) {
//                 printf("End of file reached.\n");
//             } else if (ferror(fp)) {
//                 printf("Error reading file\n");
//             }
//         }

//         printf("Contents in human-readable form:\n");
//         for (size_t i = 0; i < bytes_read; i++) {
//             if (isprint(buffer[i])) {
//                 printf("%c", buffer[i]);
//             } else {
//                 ;
//             }
//             if ((i + 1) % 16 == 0) printf("\n");
//         }
//         printf("\n");

//         printf("Contents in hexadecimal:\n");
//         for (size_t i = 0; i < bytes_read; i++) {
//             printf("%02X ", buffer[i]);
//             if ((i + 1) % 16 == 0) printf("\n");
//         }
//         printf("\n");

//         free(buffer);
//         fseek(fp, chunk_start, SEEK_SET);
//         printf("File pointer moved back to: %ld\n", chunk_start);
// }



void load_chunk(FILE *fp, int16_t *coords, int* N) {
    char coord_buffer[24]; // 24 bytes

    // while we have less than max cells
    while (*N / 3 < MAX_CELLS) {

        // read 24 bytes at a time (one row)
        int read_chars = fread(coord_buffer, sizeof(char), 24, fp);
        if (read_chars < 24) break;

        parse_coord_row(coord_buffer, &coords[*N]);
        (*N) += 3;

    }


}


int main(int argc, char *argv[]) {
    int num_threads = 1;
    // Parse command line arguments for number of threads
    for (int arg = 1; arg < argc; ++arg) {
        if (strncmp(argv[arg], "-t", 2) == 0) {
            num_threads = atoi(argv[arg] + 2);
            if (num_threads <= 0) {
                fprintf(stderr, "Invalid number of threads\n");
                return EXIT_FAILURE;
            }
        }
    }

    omp_set_num_threads(num_threads);

    // Allocate memory for coordinates
    int16_t *coords = (int16_t *)malloc(MAX_CELLS * sizeof(int16_t) * 6);
    int16_t *coords2 = coords + (MAX_CELLS * 3 * sizeof(int16_t));

    // the full coors is an array of coords 1 and coords 2. coords 2 is only a pointer but will reference the second chunk of coords

    if (!coords) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    // Open file "cells" for reading
    FILE *fp = fopen(CELL_FILE, "r");

    int line_length = 24;
    fseek(fp, 0L, SEEK_END);
    long int file_size = ftell(fp);
    printf("Size of file: %ld\n", file_size);
    fseek(fp, 0L, SEEK_SET);


    int num_chunks = (file_size + (line_length * MAX_CELLS) - 1) / (line_length * MAX_CELLS);
    int last_chunk_size = (file_size - (num_chunks - 1) * line_length * MAX_CELLS) / line_length;
    printf("Number of chunks: %d\n", num_chunks);
    printf("Last chunk size: %d\n", last_chunk_size);

    if (!fp) {
        fprintf(stderr, "Failed to open file 'cells'\n");
        free(coords);
        return EXIT_FAILURE;
    }

    int N = 0;

    printf("Number of cells: %d\n", N / 3);


    // Initialize counts arrays per thread
    int num_actual_threads = num_threads;
    #pragma omp parallel
    {
        #pragma omp single
        num_actual_threads = omp_get_num_threads();
    }

    // Allocate counts array per thread to avoid synchronization
    long int **counts_array = (long int**)malloc(num_actual_threads * sizeof(long int*));
    if (!counts_array) {
        fprintf(stderr, "Memory allocation failed\n");
        free(coords);
        return EXIT_FAILURE;
    }
    for (int t = 0; t < num_actual_threads; ++t) {
        counts_array[t] = (long int *)calloc(MAX_DISTANCE_INDEX, sizeof(long int));
        if (!counts_array[t]) {
            fprintf(stderr, "Memory allocation failed\n");
            free(coords);
            return EXIT_FAILURE;
        }
    }



    // Compute distances and counts in parallel

    for (int i = 0; i < num_chunks; i++) {
        load_chunk(fp, coords, &N);

        if (N % 3 != 0) {
        fprintf(stderr, "Incomplete coordinate data\n");
        free(coords);
        return EXIT_FAILURE;
    }

        int num_cells = N / 3; // Each cell has 3 coordinates

        if (num_cells == 0) {
            fprintf(stderr, "No coordinates were read\n");
            free(coords);
            return EXIT_FAILURE;
        }

        if (i == num_chunks - 1) {
            calculate_distances_in_chunk(coords, last_chunk_size, counts_array);
        } else {
            calculate_distances_in_chunk(coords, num_cells, counts_array);
        }

        for (int j = i + 1; j < num_chunks; j++) {
       	    /// något knas här
            fseek(fp, line_length * MAX_CELLS * j, SEEK_SET);
            load_chunk(fp, coords, &N);

            if (j == num_chunks - 1) {
                calculate_distances_between_chunks(coords, coords2, num_cells, last_chunk_size, counts_array);
            } else {
                calculate_distances_between_chunks(coords, coords2, num_cells, num_cells, counts_array);
            }
        }






    }

    fclose(fp);


    // Combine counts arrays
    long int *final_counts = (long int *)calloc(MAX_DISTANCE_INDEX, sizeof(long int));
    if (!final_counts) {
        fprintf(stderr, "Memory allocation failed\n");
        free(coords);
        return EXIT_FAILURE;
    }
    for (int t = 0; t < num_actual_threads; ++t) {
        for (int i = 0; i < MAX_DISTANCE_INDEX; ++i) {
            final_counts[i] += counts_array[t][i];
        }
        free(counts_array[t]);
    }
    free(counts_array);
    free(coords);

    // Output distances and counts in sorted order
    for (int i = 0; i < MAX_DISTANCE_INDEX; ++i) {
        if (final_counts[i] > 0) {
            double distance = i * 0.01; // Convert back to actual distance
            printf("%05.2f %d\n", distance, final_counts[i]);
        }
    }
    free(final_counts);

    return EXIT_SUCCESS;
}