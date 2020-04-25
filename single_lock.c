/*
Author:         Garrett Smith
File:           single_lock.c

Sources used:
    1) http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks-usage.pdf
        a) page 8 - Concurrent Linked Lists
    2) manpages
    3) ./hand_over_hand.c

*/

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
    pthread_mutex_t* lock;
} thread_args_t;


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

// pushes a new node to be the "next" of a given node 
// (intend use: given node = tail of linked list)
// returns node that was pushed
node_t* push(void* n) {
    node_t* last_node = (node_t*) n;
    node_t* tmp = create_and_init_node();
    tmp->data = 0;
    tmp->next = NULL;
    last_node->next = tmp;
    return tmp;
}

void insert_data(int data, node_t* curr_node) {
    curr_node->data = data;
}

// returns the data for a node at a given address
int get_node_data(node_t* target_node) {
    return target_node->data;
}

void insert_loop(counter_t* counter, node_t* node, int limit, pthread_mutex_t* lock) {
    int new_data;
    node_t* curr_node = node;
    while(get_counter(counter) < limit && curr_node != NULL) {
        new_data = (int) rand();
        pthread_mutex_lock(lock);
        insert_data(new_data, curr_node);         
        pthread_mutex_unlock(lock);
        curr_node = traverse(curr_node);
        increment_counter(counter);
    }
}

void get_loop(counter_t* counter, node_t* node, int limit, pthread_mutex_t* lock) {
    node_t* curr_node = node;
    while(get_counter(counter) < limit) {
        pthread_mutex_lock(lock);
        get_node_data(curr_node);
        pthread_mutex_unlock(lock);
        curr_node = traverse(curr_node);
        increment_counter(counter);
    }
}

void insert_job(void* args) {
    thread_args_t* targs = (thread_args_t*) args;

    counter_t* counter = targs->counter;
    node_t* node = targs->node;
    int limit = targs->limit;
    pthread_mutex_t* lock = targs->lock;

    insert_loop(counter, node, limit, lock);
}

void get_job(void* args) {
    thread_args_t* targs = (thread_args_t*) args;
    
    counter_t* counter = targs->counter;
    node_t* node = targs->node;
    int limit = targs->limit;
    pthread_mutex_t* lock = targs->lock;

    get_loop(counter, node, limit, lock);
}

// Test one: "Starting with an empty list, two threads running at the same time insert 1 million random integers 
// each on the same list."
void test_one() {
    pthread_mutex_t lock;
    node_t* head = create_and_init_node();
    counter_t* counter = create_and_init_counter();
    thread_args_t targs;

    pthread_mutex_init(&lock, NULL);

    // Used for traversal purposes. Points to the node that the program is currently "looking at"
    node_t* current_node = head;

    // set up our empty list
    for (int i = 0; i < LIMIT; i++) {
        current_node = push(current_node);
    }

    current_node = head;

    targs.counter = counter;
    targs.node = current_node;
    targs.limit = LIMIT;
    targs.lock = &lock;

    // start our second thread
    pthread_t insert_thread;
    pthread_create(&insert_thread, NULL, insert_job, &targs);

    // do the same thing on our first thread
    insert_loop(targs.counter, targs.node, targs.limit, targs.lock);

    pthread_join(insert_thread, NULL);

    printf("Test 1 - final insert counter: %d\n", targs.counter->value);
}

// Test two: "Starting with an empty list, one thread inserts 1 million random 
// integers, while another thread looks up 1 million random integers at the same time."
void test_two() {
    pthread_mutex_t lock;
    node_t* head = create_and_init_node();

    counter_t* insert_counter = create_and_init_counter();
    counter_t* get_counter = create_and_init_counter();
    
    thread_args_t insert_targs;
    thread_args_t get_targs;

    pthread_mutex_init(&lock, NULL);

    // Used for traversal purposes. Points to the node that the program is currently "looking at"
    node_t* current_node = head;

    // set up our empty list
    for (int i = 0; i < LIMIT; i++) {
        current_node = push(current_node);
    }

    current_node = head;

    insert_targs.counter = insert_counter;
    insert_targs.node = current_node;
    insert_targs.limit = LIMIT;
    insert_targs.lock = &lock;

    get_targs.counter = get_counter;
    get_targs.node = current_node;
    get_targs.limit = LIMIT;
    get_targs.lock = &lock;

    // start our second thread
    pthread_t insert_thread;
    pthread_create(&insert_thread, NULL, insert_job, &insert_targs);

    // do the same thing on our first thread
    get_loop(get_targs.counter, get_targs.node, get_targs.limit, get_targs.lock);

    pthread_join(insert_thread, NULL);

    // we can risk printing the value directly here, as we should be done with 
    // both jobs
    printf("Test 2 - final insert counter: %d\n",  insert_targs.counter->value);
    printf("Test 2 - final get counter: %d\n",  get_targs.counter->value);
}

int main() {
    srand(time(NULL));

    test_one();
    test_two();

    return 0;
}