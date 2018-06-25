#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define NUM_PAGES 50000

/* A program that accesses a new page each time
   that (given a small memory size) is most likely not on physical memory. */

int main() {
    int page_size = getpagesize();

    // Allocate a large space for data
    int data_size = NUM_PAGES * page_size;
    char *large_data_zone = malloc(data_size);

    if (large_data_zone == NULL) {
        perror("Failed to allocate space for data\n");
        exit(1);
    }

    // Read and Write happens every two pages one after the other
    int page_idx;
    for (page_idx = 0; page_idx < NUM_PAGES; page_idx++) {
        if(page_idx%2) large_data_zone[page_idx * page_size] = 0;
        else (void) large_data_zone[page_idx * page_size];
    }

    return 0;
}