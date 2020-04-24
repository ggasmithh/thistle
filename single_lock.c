#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define LIMIT (10)

typedef struct node {
    int data;
    struct node* next;
} node_t;

typedef struct counter {
    int volatile value;
} counter_t;

typedef struct thread_args {
    node_t* node;
    counter_t* counter;
    int limit;
} thread_args_t;

typedef struct linked_list {
    pthread_mutex_t lock;
} linked_list_t;

counter_t* create_and_init_counter() {
    counter_t* c = (counter_t*) calloc(1, sizeof(counter_t));
    c->value = 0;
    return c;
}

node_t* create_and_init_node() {
    node_t* n = (node_t*) calloc(1, sizeof(node_t));
    return n;
}

void increment_counter(counter_t* c) {
    c->value++;
}

int get_counter(counter_t* c) {
    int value = c->value;
    return value;
}

void decrement_counter(counter_t* c) {
    c->value--;
}

// returns pointer to next node
node_t* traverse(void* n) {
    if (n != NULL) {
        node_t* curr_node = (node_t*) n;
        return curr_node->next;
    } 
}