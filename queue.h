#ifndef HEADER_FILE
#define HEADER_FILE


typedef struct element {
    char *operation;
}element;

typedef struct queue {
    int size;
    int to_get; // Pointer to next occupied slot
    int to_put; // Pointer to next empty slot
    int elems_in_queue;
    struct element **operation_queue; // Array of strings that will store the operations

}queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

element *element_init (char * operation);

#endif
