#include <stdio.h>
#include <pthread.h>

typedef struct node {
    int data;
    struct node_t* next;
    pthread_mutex_t lock;
} node_t;

// releases lock on current node, acquires lock on next node, returns pointer to next node
void *traverse(void* node) {
    node_t* curr_node = (node_t *) node;
    node_t* next_node = (node_t *) &curr_node->next;
    pthread_mutex_unlock(&curr_node->lock);
    pthread_mutex_lock(&next_node->lock);
}
