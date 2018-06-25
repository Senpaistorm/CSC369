#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#define LINE_LIMIT  100


extern int memsize;

extern int debug;

extern struct frame *coremap;

extern char *tracefile;

int addrs_count;
int cur_addr;
int *addr_in_use;
int *addr_space;
int *nearest_future_use;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	int frame = 0, optimal = 0;
	int i, j, k;
	// Get first future index of the same address being used 
	// for the nearest_future_use list
	for(i=0; i<memsize; i++){
		for(j = cur_addr; j < addrs_count; j++){
			// Find the index of the first use in the future
			if(addr_in_use[i] == addr_space[j]){
				nearest_future_use[i] = j;
				break;
			}
		}
		// Reach the end of the tracefile and the address stored in this frame
		// will never be used again, simply return this frame
		if(j == addrs_count){
			return i;
		}
	}
	// Iterate the nearest_future_use list to compare which page would not be 
	// used for the longest time and evict that corresponding frame
	for(k=0;k<memsize;k++){
		if(nearest_future_use[k] > optimal){
			frame = k;
			optimal = nearest_future_use[k];
		}
	}
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	// Set currently used address for the frame, syncronized with the trace file
	addr_in_use[p->frame>>PAGE_SHIFT] = addr_space[cur_addr];
	cur_addr = cur_addr + 1;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	// Initialization
	cur_addr = 0;
	nearest_future_use = malloc(sizeof(int) * memsize);
	addr_in_use = malloc(sizeof(int) * memsize);
	addr_t addr = 0;
	FILE *fp;
	char buffer[LINE_LIMIT];
	int i = 0;
	char type;
	
	// Open the trace file
	fp = fopen(tracefile, "r");
	if(fp == NULL){
		perror("Failed to open the trace file");
		exit(1);
	}
	// Count number of addresses in the trace file
	while(fgets(buffer, LINE_LIMIT, fp) != NULL){
		addrs_count = addrs_count + 1;
	}
	// Initialize the address space for the trace file
	addr_space = malloc(sizeof(int) * addrs_count);

	// Reset file pointer to the beginning of the file
	rewind(fp);

	// Build up the address space for the trace file, address stored
	// in addr_space
	while(fgets(buffer, LINE_LIMIT, fp)!=NULL){
		if(buffer[0] != '='){
			sscanf(buffer, "%c %lx", &type, &addr);
			addr_space[i] = addr;
			i++;
		}
	}
}

