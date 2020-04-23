#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct __node_t {
    int data;
    struct __node_t* next;
    pthread_mutex_t lock;
} node_t;

typedef struct __linked_list {
    node_t* tail;
    node_t* head;
} linked_list_t;

void* create_and_init_node() {
    node_t* node = calloc(1, sizeof(node_t));
    pthread_mutex_init(&node->lock, NULL);
    return node;
}

void init_linked_list(void* l) {
    linked_list_t* linked_list = (linked_list_t*) l;
    node_t* tmp = (node_t*) create_and_init_node();
    tmp->next = (node_t*) linked_list->tail;
    linked_list->head = tmp;
    linked_list->tail = tmp;
}

// releases lock on current node, acquires lock on next node, returns pointer to next node
void* traverse(void* n) {
    node_t* curr_node = (node_t*) n;
    node_t* next_node = (node_t*) &curr_node->next;
    
    pthread_mutex_unlock(&curr_node->lock);
    pthread_mutex_lock(&next_node->lock);
    
    return next_node;
}

// returns the data for a node at a given address
int get_node_data(void* n) {
     node_t* node = (node_t*) n;
    return node->data;
}

// pushes a node with the given data to the end of the given linked list and updates its tail
void push(int data, void* l) {
    linked_list_t* linked_list = (linked_list_t*) l;
    node_t* tmp = (node_t*) create_and_init_node();
    tmp->data = data;
    linked_list->tail->next = tmp;
    linked_list->tail = tmp;
}

int main() {
    linked_list_t* linked_list;
    node_t* current_node;

    init_linked_list(linked_list);
    current_node = linked_list->head;

    for (int i = 0; i < 10; i++) {
        push(i, linked_list);
    }

    for (int i = 0; i < 10; i++) {
        current_node = traverse(current_node);
        printf("got number %d", get_node_data(current_node));
    }

    return 0;
}
