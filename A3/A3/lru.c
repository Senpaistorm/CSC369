#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

struct node{
	int frame;
	struct node* next;
};

// Linked list keeping track of least recently frame in insertion order
struct node *head;

int pop_lru(){
	// Remove the frame of first node in the linked list if head is not 
	// NULL else 0
    	if(head == NULL){
        		return 0;
    	}else{
        		int frame = head->frame;
        		struct node *temp = head;
        		head = head->next;
        		free(temp);
        		return frame;
	}
}

void create_new_frame(int frame){
	struct node *temp,*cur_node;
	// Malloc space for new node in the linked list
	temp= (struct node *)malloc(sizeof(struct node));
	temp->frame=frame;
	temp->next = NULL;
	if(head != NULL){
		cur_node= head;
		// Go to the last node
		while(cur_node->next != NULL){
			cur_node=cur_node->next;
		}
		// Insert the new node at the end of the linked list
		cur_node->next =temp;
	}else{
		// If head is NULL, head is set to be the new node
		head = temp;
    }
}
 
void remove_used_frame(int frame){
	struct node *temp, *prev;
	temp=head;
	while(temp!=NULL){
		// Find a frame that has already been used, mark it as not
		// least recently used by removing it from the linked list
		if(temp->frame==frame){
			// If it is head ,no prev node found, remove head and 
			// move one node ahead
			if(temp==head){
				head=temp->next;
				free(temp);
			// Else, remove the one matched the frame and keep 
			// the linked list continuous
			}else{
				prev->next=temp->next;
				free(temp);
			}
		}
		prev=temp;
		temp= temp->next;
	}
}

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	// Always return the head element of the linked list because it is
	// least recently used
 	return pop_lru();
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int frame = p->frame >> PAGE_SHIFT;
	// Remove frame being used from the linked list
	remove_used_frame(frame);
	// Insert a new node with frame into the linked list
	create_new_frame(frame);
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	head = NULL;
}
