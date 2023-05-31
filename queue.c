//SSOO-P3 2022-2023

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

//To create an element
struct  element *element_init (char *operation) {
    struct element* our_element = (struct element*) malloc(sizeof (struct element));
    our_element -> operation = operation; // Nos ahorramos el strcopy, simplemente no free memoria
    return our_element;
}

//To create a queue
queue* queue_init(int size){

    if (size < 1) {
        printf("[ERROR] Size of queue must be greater than 0\n");
        exit(-1);
    }
	queue * q = (queue *)malloc(sizeof(queue));
    q -> elems_in_queue = 0;
    q -> to_put = 0;
    q -> size = size;
    q -> operation_queue = (struct element**) malloc(sizeof(struct element*) * size);
	return q;
}
// [ ][ ]
//  t  e
// To Enqueue an element
int queue_put(queue *q, struct element* x) {
    if (q -> elems_in_queue == q -> size) { // is full
        fprintf(stderr, "[ERROR] Tried to put but operation queue is full\n");
        printf("Tried to put but operation queue is full\n");
        return -1;
    }
    q -> operation_queue[q -> to_put] = x;
    q -> elems_in_queue++;
    q -> to_put = (q -> to_put+1) % (q -> size);
	return 0;
}


struct element* queue_get(queue *q) {
	struct element* element_return;
    if (q -> elems_in_queue == 0) {
        fprintf(stderr, "[ERROR] Tried to get but operation queue is empty\n");
        return NULL;
    }
    element_return = q->operation_queue[q -> to_get];
    q->elems_in_queue--;
    q->to_get  = (q-> to_get + 1) % (q->size);
	return element_return;
}


//To check queue state
int queue_empty(queue *q){
	return q ->elems_in_queue == 0;
}

int queue_full(queue *q){
    return q ->elems_in_queue == q->size;
}

//To destroy the queue and free the resources
int queue_destroy(queue *q){
	free(q);
    return 0;
}
