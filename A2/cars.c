#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "traffic.h"

extern struct intersection isection;

/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
void parse_schedule(char *file_name) {
    int id;
    struct car *cur_car;
    struct lane *cur_lane;
    enum direction in_dir, out_dir;
    FILE *f = fopen(file_name, "r");

    /* parse file */
    while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {

        /* construct car */
        cur_car = malloc(sizeof(struct car));
        cur_car->id = id;
        cur_car->in_dir = in_dir;
        cur_car->out_dir = out_dir;

        /* append new car to head of corresponding list */
        cur_lane = &isection.lanes[in_dir];
        cur_car->next = cur_lane->in_cars;
        cur_lane->in_cars = cur_car;
        cur_lane->inc++;
    }

    fclose(f);
}

/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */
void init_intersection() {
    int i;
    for(i=0;i<4;i++){
        //Initialize all the threads in four quadrants and four lanes
        pthread_mutex_init(&(isection.quad[i]), NULL);
        pthread_mutex_init(&(isection.lanes[i].lock), NULL);
        pthread_cond_init(&(isection.lanes[i].producer_cv), NULL);
        pthread_cond_init(&(isection.lanes[i].consumer_cv), NULL);
        //Initialize all the attributes in four lanes
        isection.lanes[i].in_cars = NULL;
        isection.lanes[i].out_cars = NULL;
        isection.lanes[i].inc = 0;
        isection.lanes[i].passed = 0;
        isection.lanes[i].buffer = malloc(LANE_LENGTH * sizeof(struct car *));
        isection.lanes[i].head = 0;
        isection.lanes[i].tail = 0;
        isection.lanes[i].capacity = LANE_LENGTH;
        isection.lanes[i].in_buf = 0;
    }
}

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {
    struct lane *l = arg;
    
    /* avoid compiler warning */
    l = l;
    pthread_mutex_lock(&l->lock);
    //To check if any car is pending to enter the intersection
    while(l->inc>0){
        //While buffer is full we need to wait for car_cross(consumer)
        while(l->in_buf == l->capacity){
            pthread_cond_wait(&l->producer_cv, &l->lock);
        }
        //l-tail points to the last car in the buffer that should be
        //pushed into the intersection
        l->buffer[l->tail]=l->in_cars;
        l->tail = (l->tail+1)%l->capacity;
        l->in_buf++;
        l->inc--;
        l->in_cars = l->in_cars->next;
        pthread_cond_signal(&l->consumer_cv);
    }
    pthread_mutex_unlock(&l->lock);
    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list of the lane that corresponds to the car's
 * out_dir. Do not free the cars!
 *
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {
    struct lane *l = arg;
    /* avoid compiler warning */
    l = l;
    int i;
    int j;
    struct car *cur_car;
    pthread_mutex_lock(&l->lock);
    //To check if there is still any car need to cross the interseciton
    while(l->inc!=0 || l->in_buf!=0){
        //If the buffer is empty wait for the car_arrive(producer)
        while(l->in_buf == 0){
            pthread_cond_wait(&l->consumer_cv, &l->lock);
        }
        //l-head points to the first car in the buffer that need to cross 
        //the intersection
        cur_car = l->buffer[l->head];
        l->head = (l->head+1)%l->capacity;
        l->passed++;
        l->in_buf--;
        //Compute the quadrants that the car needs to occupy
        int *dir = compute_path(cur_car->in_dir, cur_car->out_dir);
        i = 0;
        //Lock the quadrants before entering the interseciton
        while(dir[i]!=-1){
            pthread_mutex_lock(&isection.quad[dir[i]-1]);
            i++;
        }
        //Print the information of the car
        printf("%d %d %d\n", cur_car->in_dir, cur_car->out_dir, cur_car->id);
        cur_car->next = isection.lanes[cur_car->out_dir].out_cars;
        isection.lanes[cur_car->out_dir].out_cars = cur_car;
        j = 0;
        //Unlock the quadrants after exiting the interseciton
        while(dir[j]!=-1){
            pthread_mutex_unlock(&isection.quad[dir[j]-1]);
            j++;
        }
        free(dir);
        pthread_cond_signal(&l->producer_cv);
    }
    free(l->buffer);
    pthread_mutex_unlock(&l->lock); 
    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Given a car's in_dir and out_dir return a sorted 
 * list of the quadrants the car will pass through.
 * 
 */
int *compute_path(enum direction in_dir, enum direction out_dir) {
    int *dir= malloc(sizeof(int) * 4);
    if(!dir){
        perror("malloc failed");
        exit(1);
    }
    int i;
    //Initialize the quad number with -1
    for(i=0;i<4;i++){
        dir[i] = -1;
    }
    //Check the in_dir
    if(in_dir == EAST){
        //Check the out_dir and assign the value to indicate which quadrants
        //should be occupied
        if(out_dir == EAST){
            dir[0] = 1;
            dir[1] = 4;
        }
        if(out_dir == WEST){
            dir[0] = 1;
            dir[1] = 2;
        }
        if(out_dir == SOUTH){
            dir[0] = 1;
            dir[1] = 2;
            dir[2] = 3;
        }
        if(out_dir == NORTH){
            dir[0] = 1;
        }
    }
    if(in_dir == WEST){
        if(out_dir == EAST){
            dir[0] = 3;
            dir[1] = 4;
        }
        if(out_dir == WEST){
            dir[0] = 2;
            dir[1] = 3;
        }
        if(out_dir == SOUTH){
            dir[0] = 3;
        }
        if(out_dir == NORTH){
            dir[0] = 1;
            dir[1] = 3;
            dir[2] = 4;
        }
    }
    if(in_dir == SOUTH){
        if(out_dir == EAST){
            dir[0] = 4;
        }
        if(out_dir == WEST){
            dir[0] = 1;
            dir[1] = 2;
            dir[2] = 4;
        }
        if(out_dir == SOUTH){
            dir[0] = 3;
            dir[1] = 4;
        }
        if(out_dir == NORTH){
            dir[0] = 1;
            dir[1] = 4;
        }
    }
    if(in_dir == NORTH){
        if(out_dir == EAST){
            dir[0] = 2;
            dir[1] = 3;
            dir[2] = 4;
        }
        if(out_dir == WEST){
            dir[0] = 2;
        }
        if(out_dir == SOUTH){
            dir[0] = 2;
            dir[1] = 3;
        }
        if(out_dir == NORTH){
            dir[0] = 1;
            dir[1] = 2;
        }
    }
    return dir;
}
