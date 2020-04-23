#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// only using non-stdint.h types in this b/c its required in the spec lol

// a good deal of code is from OSTEP, specifically the following chapter:
// http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks-usage.pdf

#define LIMIT (1000000)

typedef struct node {
    int data;
    struct node* next;
    pthread_mutex_t lock;
} node_t;

typedef struct counter {
    int value;
    pthread_mutex_t lock;
} counter_t;

typedef struct thread_args {
    node_t* node;
    counter_t* counter;
    int limit;
} thread_args_t;

counter_t* create_and_init_counter() {
    counter_t* c = (counter_t*) calloc(1, sizeof(counter_t));
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
    return c;
}

node_t* create_and_init_node() {
    node_t* n = (node_t*) calloc(1, sizeof(node_t));
    pthread_mutex_init(&n->lock, NULL);
    return n;
}

void increment_counter(counter_t* c) {
    pthread_mutex_lock(&c->lock);
    c->value++;
    pthread_mutex_unlock(&c->lock);
}

int get_counter(counter_t* c) {
    pthread_mutex_lock(&c->lock);
    int value = c->value;
    pthread_mutex_unlock(&c->lock);
    return value;
}

void decrement_counter(counter_t* c) {
        pthread_mutex_lock(&c->lock);
    c->value--;
    pthread_mutex_unlock(&c->lock);
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

// pushes a new node to be the "next" of a given node 
// (intend use: given node = tail of linked list)
// returns node that was pushed
node_t* push(void* n) {
    node_t* last_node = (node_t*) n;
    node_t* tmp = create_and_init_node();
    tmp->data = 0;
    tmp->next = NULL;
    pthread_mutex_lock(&last_node->lock);
    last_node->next = tmp;
    pthread_mutex_unlock(&last_node->lock);
    return tmp;
}

void insert_data(void* d, void *n) {
    node_t* curr_node = (node_t*) n;
    int new_data = *(int*) d;
    pthread_mutex_lock(&curr_node->lock);
    curr_node->data = new_data;
    pthread_mutex_unlock(&curr_node->lock);
}

void insert_loop(counter_t* counter, node_t* node, int limit) {
    int new_data;
    while(get_counter(counter) < limit) {
        new_data = (int) rand();
        insert_data(&new_data, node);
        node = traverse(node);
        increment_counter(counter);
    }
}

void get_loop(counter_t* counter, node_t* node, int limit) {
    node_t* curr_node = node;
    while(get_counter(counter) < limit && curr_node != NULL) {
        get_node_data(curr_node);
        curr_node = traverse(curr_node);
        increment_counter(counter);
    }
}

void insert_job(void* args) {
    thread_args_t* targs = (thread_args_t*) args;

    counter_t* counter = targs->counter;
    node_t* node = targs->node;
    int limit = targs->limit;

    insert_loop(counter, node, limit);
}

// Test one: "Starting with an empty list, two threads running at the same time insert 1 million random integers 
// each on the same list."
void test_one() {
    node_t* head = create_and_init_node();
    counter_t* counter = create_and_init_counter();
    thread_args_t targs;

    // Used for traversal purposes. Points to the node that the program is currently "looking at"
    // Speeds up push() too
    node_t* current_node = head;

    targs.counter = counter;
    targs.node = current_node;
    targs.limit = LIMIT;

    // set up our empty list
    for (int i = 0; i++; i < LIMIT) {
        current_node = push(current_node);
    }

    current_node = head;

    // start our second thread
    pthread_t insert_thread;
    pthread_create(&insert_thread, NULL, insert_job, &targs);

    // do the same thing on our first thread
    insert_loop(targs.counter, targs.node, targs.limit);

    if (pthread_join(insert_thread, NULL)) {
        printf("oops");
        return 1;
    }

    current_node = head;

    return 0;
}

// Test two: "Starting with an empty list, one thread inserts 1 million random 
// integers, while another thread looks up 1 million random integers at the same time."
void test_two() {
    node_t* head = create_and_init_node();

    counter_t* insert_counter = create_and_init_counter();
    counter_t* get_counter = create_and_init_counter();

    thread_args_t insert_targs;
    thread_args_t get_targs;

    // Used for traversal purposes. Points to the node that the program is currently "looking at"
    // Speeds up push() too
    node_t* current_node = head;

    insert_targs.counter = insert_counter;
    insert_targs.node = current_node;
    insert_targs.limit = LIMIT;

    get_targs.counter = get_counter;
    get_targs.node = current_node;
    get_targs.limit = LIMIT;

    // start our second thread
    pthread_t insert_thread;
    pthread_create(&insert_thread, NULL, insert_job, &insert_targs);

    // do the same thing on our first thread
    get_loop(get_targs.counter, get_targs.node, get_targs.limit);

    if (pthread_join(insert_thread, NULL)) {
        printf("oops2");
        return 1;
    }

    current_node = head;

    // we can risk printing the value directly here, as we should be done with 
    // both jobs
    printf("final insert counter: %d\n",  insert_targs.counter->value);
    printf("final get counter: %d\n",  get_targs.counter->value);

    return 0;
}

int main() {
    srand(time(NULL));

    //test_one();
    test_two();

    return 0;
}
