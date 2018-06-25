#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

// Index of the frame
int clock_hand;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	while (clock_hand <= memsize){

		// Advance hand by 1 in every eviction
		clock_hand = (clock_hand + 1) % memsize;

		// If the page frame is free to use then return the frame number
		// indicated by clock_hand
		if(!(coremap[clock_hand].pte->frame & PG_REF)){
		    break;
		}

		// Else give the page frame a second chance by setting
		// the REF bit of the frame off 
		coremap[clock_hand].pte->frame &= ~PG_REF;

	}
	return clock_hand;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	// Set REF bit if the page is being referenced
	p->frame |= PG_REF;
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clock_hand = 0;
}
