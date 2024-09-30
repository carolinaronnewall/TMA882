#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>

#define MAX_COORDS 330000  // Maximum number of coordinates
#define MAX_DISTANCE_INDEX 3500  // Adjust based on expected maximum distance
#define CELL_FILE "cells"

// Function to parse coordinate from string
static inline int16_t parse_coord(const char *ptr) {
    // ptr points to start of coordinate (+dd.ddd)
    // ptr[0]: sign '+' or '-'
    // ptr[1], ptr[2]: digits
    // ptr[3]: '.'
    // ptr[4], ptr[5], ptr[6]: digits

    int sign = (ptr[0] == '-') ? -1 : 1;

    int16_t integer_part = (ptr[1] - '0') * 10 + (ptr[2] - '0'); // two digits
    int16_t fractional_part = (ptr[4] - '0') * 100 + (ptr[5] - '0') * 10 + (ptr[6] - '0'); // three digits

    int16_t value = integer_part * 1000 + fractional_part;
    value *= sign;

    return value;
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
    int16_t *coords = (int16_t *)malloc(MAX_COORDS * sizeof(int16_t));
    if (!coords) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    // Open file "cells" for reading
    FILE *fp = fopen(CELL_FILE, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open file 'cells'\n");
        free(coords);
        return EXIT_FAILURE;
    }

    // Read and parse coordinates
    char coord_buffer[9]; // 8 chars + null terminator
    int N = 0; // number of coordinates read
    int ch;

    while (N < MAX_COORDS) {
        // Skip any whitespace characters
        do {
            ch = fgetc(fp);
            if (ch == EOF) break;
        } while (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r');

        if (ch == EOF) break;
        ungetc(ch, fp);  // Put the non-whitespace character back

        // Read coordinate (expecting exactly 8 characters)
        int read_chars = fread(coord_buffer, 1, 8, fp);
        if (read_chars < 8) break;
        coord_buffer[8] = '\0';  // Null-terminate the string

        coords[N++] = parse_coord(coord_buffer);
    }
    fclose(fp);

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

    // Initialize counts arrays per thread
    int num_actual_threads = num_threads;
    #pragma omp parallel
    {
        #pragma omp single
        num_actual_threads = omp_get_num_threads();
    }

    // Allocate counts array per thread to avoid synchronization
    int16_t **counts_array = (int16_t **)malloc(num_actual_threads * sizeof(int16_t *));
    if (!counts_array) {
        fprintf(stderr, "Memory allocation failed\n");
        free(coords);
        return EXIT_FAILURE;
    }
    for (int t = 0; t < num_actual_threads; ++t) {
        counts_array[t] = (int16_t *)calloc(MAX_DISTANCE_INDEX, sizeof(int16_t));
        if (!counts_array[t]) {
            fprintf(stderr, "Memory allocation failed\n");
            free(coords);
            return EXIT_FAILURE;
        }
    }

    // Compute distances and counts in parallel
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_cells - 1; ++i) {
        int tid = omp_get_thread_num();
        int16_t *local_counts = counts_array[tid];
        int16_t xi = coords[i * 3 + 0];
        int16_t yi = coords[i * 3 + 1];
        int16_t zi = coords[i * 3 + 2];
        for (int j = i + 1; j < num_cells; ++j) {
            int16_t dx = xi - coords[j * 3 + 0];
            int16_t dy = yi - coords[j * 3 + 1];
            int16_t dz = zi - coords[j * 3 + 2];

            int32_t dist_sq = (int32_t)dx * dx + (int32_t)dy * dy + (int32_t)dz * dz;
            double dist = sqrt((double)dist_sq);

            int distance_scaled = (int)(dist / 10 + 0.5); // Scale to two decimal places¨

            if (distance_scaled >= 0 && distance_scaled < MAX_DISTANCE_INDEX) {
                local_counts[distance_scaled]++;
            }
        }
    }

    // Combine counts arrays
    int16_t *final_counts = (int16_t *)calloc(MAX_DISTANCE_INDEX, sizeof(int16_t));
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
