#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>
#include <stdint.h>
#include <errno.h>

#define MAX_DISTANCE_INDEX 3464  // Maximum distance is 34.63 units, scaled to two decimal places
#define CELL_FILE "cells"
#define MAX_THREADS 256  

// Function to parse a single coordinate from string
static inline int16_t parse_single_coord(const char *ptr) {
    int sign = (ptr[0] == '-') ? -1 : 1;

    int integer_part = (ptr[1] - '0') * 10 + (ptr[2] - '0'); // two digits
    int fractional_part = (ptr[4] - '0') * 100 + (ptr[5] - '0') * 10 + (ptr[6] - '0'); // three digits

    return (int16_t)(sign * (integer_part * 1000 + fractional_part));
}

// Function to parse a row of coordinates
static inline void parse_coord_row(const char *ptr, int16_t *coords) {
    for (int i = 0; i < 3; i++) {
        coords[i] = parse_single_coord(ptr);
        ptr += 8; // Move past the current coordinate and the space
    }
}

// Function to calculate distances within a single chunk
void calculate_distances_in_chunk(const int16_t *coords, int num_cells, long int *counts) {
    #pragma omp parallel
    {
        // Allocate a private counts array for each thread
        long int local_counts[MAX_DISTANCE_INDEX] = {0};

        #pragma omp for schedule(dynamic)
        for (int i = 0; i < num_cells - 1; ++i) {
            int16_t xi = coords[i * 3];
            int16_t yi = coords[i * 3 + 1];
            int16_t zi = coords[i * 3 + 2];

            for (int j = i + 1; j < num_cells; ++j) {
                int16_t dx = xi - coords[j * 3];
                int16_t dy = yi - coords[j * 3 + 1];
                int16_t dz = zi - coords[j * 3 + 2];

                int32_t dist_sq = (int32_t)dx * dx + (int32_t)dy * dy + (int32_t)dz * dz;
                double dist = sqrt((double)dist_sq);

                int16_t distance_scaled = (int16_t)(dist / 10.0 + 0.5); // Scale to two decimal places

                if (distance_scaled >= 0 && distance_scaled < MAX_DISTANCE_INDEX) {
                    local_counts[distance_scaled]++;
                }
            }
        }

        // Merge local counts into global counts
        #pragma omp critical
        {
            for (int k = 0; k < MAX_DISTANCE_INDEX; ++k) {
                counts[k] += local_counts[k];
            }
        }
    }
}

// Function to calculate distances between two different chunks
void calculate_distances_between_chunks(const int16_t *coords1, const int16_t *coords2, int num_cells1, int num_cells2, long int *counts) {
    #pragma omp parallel
    {
        // Allocate a private counts array for each thread
        long int local_counts[MAX_DISTANCE_INDEX] = {0};

        #pragma omp for schedule(dynamic)
        for (int i = 0; i < num_cells1; ++i) {
            int16_t xi = coords1[i * 3];
            int16_t yi = coords1[i * 3 + 1];
            int16_t zi = coords1[i * 3 + 2];

            for (int j = 0; j < num_cells2; ++j) {
                int16_t dx = xi - coords2[j * 3];
                int16_t dy = yi - coords2[j * 3 + 1];
                int16_t dz = zi - coords2[j * 3 + 2];

                int32_t dist_sq = (int32_t)dx * dx + (int32_t)dy * dy + (int32_t)dz * dz;
                double dist = sqrt((double)dist_sq);

                int16_t distance_scaled = (int16_t)(dist / 10.0 + 0.5); // Scale to two decimal places

                if (distance_scaled >= 0 && distance_scaled < MAX_DISTANCE_INDEX) {
                    local_counts[distance_scaled]++;
                }
            }
        }

        // Merge local counts into global counts
        #pragma omp critical
        {
            for (int k = 0; k < MAX_DISTANCE_INDEX; ++k) {
                counts[k] += local_counts[k];
            }   
        }
    }
}

void load_chunk(FILE *fp, int16_t *coords, int max_cells, int *cells_loaded) {
    char coord_buffer[24];
    *cells_loaded = 0;

    while (*cells_loaded < max_cells) {
        size_t read_chars = fread(coord_buffer, sizeof(char), 24, fp);
        if (read_chars < 24) {
            if (feof(fp)) {
                break;
            } else {
                fprintf(stderr, "Error reading file: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        parse_coord_row(coord_buffer, &coords[*cells_loaded * 3]);
        (*cells_loaded)++;
    }
}

int main(int argc, char *argv[]) {
    int num_threads = 1;
    // Parse command line arguments for number of threads
    for (int arg = 1; arg < argc; ++arg) {
        if (strncmp(argv[arg], "-t", 2) == 0) {
            num_threads = atoi(argv[arg] + 2);
            if (num_threads <= 0 || num_threads > MAX_THREADS) {
                fprintf(stderr, "Invalid number of threads. Must be between 1 and %d.\n", MAX_THREADS);
                return EXIT_FAILURE;
            }
        }
    }

    omp_set_num_threads(num_threads);

    // Determine maximum cells per chunk to limit memory usage
    // Each cell has 3 int16_t, so 6 bytes. We use two chunks in memory at a time
    // Total memory for coordinates: 2 * MAX_CELLS_PER_CHUNK * 3 * sizeof(int16_t)
    // Let total memory <= 4 MiB to leave space for counts and other allocations
    const int MAX_CELLS_PER_CHUNK = 350000; // 350,000 cells -> 4,200,000 bytes

    // Allocate memory for two chunks
    int16_t *coords1 = (int16_t *)malloc(MAX_CELLS_PER_CHUNK * 3 * sizeof(int16_t));
    int16_t *coords2 = (int16_t *)malloc(MAX_CELLS_PER_CHUNK * 3 * sizeof(int16_t));
    if (!coords1 || !coords2) {
        fprintf(stderr, "Memory allocation failed\n");
        free(coords1);
        free(coords2);
        return EXIT_FAILURE;
    }

    // Initialize global counts
    long int *final_counts = (long int *)calloc(MAX_DISTANCE_INDEX, sizeof(long int));
    if (!final_counts) {
        fprintf(stderr, "Memory allocation failed\n");
        free(coords1);
        free(coords2);
        return EXIT_FAILURE;
    }

    // Open file "cells" for reading
    FILE *fp = fopen(CELL_FILE, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open file '%s'\n", CELL_FILE);
        free(coords1);
        free(coords2);
        free(final_counts);
        return EXIT_FAILURE;
    }

    // Determine file size
    if (fseek(fp, 0L, SEEK_END) != 0) {
        fprintf(stderr, "fseek failed\n");
        fclose(fp);
        free(coords1);
        free(coords2);
        free(final_counts);
        return EXIT_FAILURE;
    }

    long int file_size = ftell(fp);
    if (file_size == -1L) {
        fprintf(stderr, "ftell failed\n");
        fclose(fp);
        free(coords1);
        free(coords2);
        free(final_counts);
        return EXIT_FAILURE;
    }
    fseek(fp, 0L, SEEK_SET);

    // Calculate number of chunks
    int num_chunks = (file_size + (24 * MAX_CELLS_PER_CHUNK) - 1) / (24 * MAX_CELLS_PER_CHUNK);
    int last_chunk_size = (file_size % (24 * MAX_CELLS_PER_CHUNK)) / 24;
    if (last_chunk_size == 0 && num_chunks > 0) {
        last_chunk_size = MAX_CELLS_PER_CHUNK;
    }

    // Loop through each chunk
    for (int i = 0; i < num_chunks; i++) {
        // Determine current chunk size
        int current_chunk_size = (i == num_chunks - 1) ? last_chunk_size : MAX_CELLS_PER_CHUNK;

        // Load chunk i into coords1
        fseek(fp, i * 24 * MAX_CELLS_PER_CHUNK, SEEK_SET);
        int cells_loaded1;
        load_chunk(fp, coords1, current_chunk_size, &cells_loaded1);
        if (cells_loaded1 != current_chunk_size) {
            fprintf(stderr, "Unexpected number of cells loaded for chunk %d\n", i);
            fclose(fp);
            free(coords1);
            free(coords2);
            free(final_counts);
            return EXIT_FAILURE;
        }

        // Calculate distances within chunk i
        calculate_distances_in_chunk(coords1, current_chunk_size, final_counts);

        // Calculate distances between chunk i and all subsequent chunks
        for (int j = i + 1; j < num_chunks; j++) {
            int chunk_j_size = (j == num_chunks - 1) ? last_chunk_size : MAX_CELLS_PER_CHUNK;

            // Load chunk j into coords2
            fseek(fp, j * 24 * MAX_CELLS_PER_CHUNK, SEEK_SET);
            int cells_loaded2;
            load_chunk(fp, coords2, chunk_j_size, &cells_loaded2);
            if (cells_loaded2 != chunk_j_size) {
                fprintf(stderr, "Unexpected number of cells loaded for chunk %d\n", j);
                fclose(fp);
                free(coords1);
                free(coords2);
                free(final_counts);
                return EXIT_FAILURE;
            }

            // Calculate distances between chunk i and chunk j
            calculate_distances_between_chunks(coords1, coords2, current_chunk_size, chunk_j_size, final_counts);
        }
    }

    fclose(fp);
    free(coords2);
    free(coords1);

    // Output distances and counts in sorted order
    for (int i = 0; i < MAX_DISTANCE_INDEX; ++i) {
        if (final_counts[i] > 0) {
            double distance = i * 0.01; // Convert back to actual distance
            printf("%05.2f %ld\n", distance, final_counts[i]);
        }
    }

    free(final_counts);

    return EXIT_SUCCESS;
}
