#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// only using non-stdint.h types in this b/c its required in the spec lol

typedef struct node {
    int data;
    struct node* next;
    pthread_mutex_t lock;
} node_t;

node_t* create_and_init_node() {
    node_t* n = (node_t*) calloc(1, sizeof(node_t));
    pthread_mutex_init(&n->lock, NULL);
    return n;
}

// releases lock on current node, acquires lock on next node, returns pointer to next node
node_t* traverse(void* n) {
    node_t* curr_node = (node_t*) n;
    if (curr_node != NULL && curr_node->next != NULL) {
        node_t* next_node = curr_node->next;
        
        pthread_mutex_lock(&next_node->lock);
        pthread_mutex_unlock(&curr_node->lock);
        
        return next_node;
    } else {
        return NULL;
    }
}

// returns the data for a node at a given address
int get_node_data(node_t* target_node) {
    return target_node->data;
}

// pushes a node with the given data to the end of the given linked list and updates its tail
node_t* push(void* d, void* n) {
    node_t* last_node = (node_t*) n;
    int new_data = *(int*) d;
    node_t* tmp = create_and_init_node();
    tmp->data = new_data;
    tmp->next = NULL;
    pthread_mutex_lock(&last_node->lock);
    last_node->next = tmp;
    pthread_mutex_unlock(&last_node->lock);
    return tmp;
}

int main() {
    node_t* head = create_and_init_node();

    // Used for traversal purposes. Points to the node that the program is currently "looking at"
    // Speeds up push() too
    node_t* current_node = head;

    for (int i = 0; i < 10; i++) {
        current_node = push(&i, current_node);
    }

    current_node = head;

    for (int i = 0; i < 10; i++) {
        current_node = traverse(current_node);
        if (current_node != NULL) {
            printf("got number %d\n", get_node_data(current_node));
        }
    }

    return 0;
}
