#include "thistle.h"

// only using non-stdint.h types in this b/c its required in the spec lol

// a good deal of code is from OSTEP, specifically the following chapter:
// http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks-usage.pdf

counter_t* create_and_init_counter() {
    counter_t* c = (counter_t*) calloc(1, sizeof(counter_t));
    c->value = 0;
    return c;
}

node_t* create_and_init_node() {
    node_t* n = (node_t*) calloc(1, sizeof(node_t));
    pthread_mutex_init(&n->lock, NULL);
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

// releases lock on current node, acquires lock on next node, returns pointer to next node
node_t* traverse(void* n) {
    if (n != NULL) {
        node_t* curr_node = (node_t*) n;
        if (curr_node->next != NULL) {
            node_t* next_node = (node_t*) curr_node->next;
            
            //int lstatus = pthread_mutex_lock(&next_node->lock);
            //int ulstatus = pthread_mutex_unlock(&curr_node->lock);
            
            return next_node;
        }
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
    //pthread_mutex_lock(&last_node->lock);
    last_node->next = tmp;
    //pthread_mutex_unlock(&last_node->lock);
    return tmp;
}

void insert_data(int data, node_t* curr_node) {
    curr_node->data = data;
}

void insert_loop(counter_t* counter, node_t* node, int limit) {
    int new_data;
    node_t* curr_node = node;
    while(get_counter(counter) < limit && curr_node != NULL) {
        new_data = (int) rand();
        insert_data(new_data, curr_node);         
        curr_node = traverse(curr_node);
        increment_counter(counter);
    }
}

void get_loop(counter_t* counter, node_t* node, int limit) {
    node_t* curr_node = node;
    while(get_counter(counter) < limit) {
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

void get_job(void* args) {
    thread_args_t* targs = (thread_args_t*) args;
    
    counter_t* counter = targs->counter;
    node_t* node = targs->node;
    int limit = targs->limit;

    get_loop(counter, node, limit);
}

// Test one: "Starting with an empty list, two threads running at the same time insert 1 million random integers 
// each on the same list."
void test_one() {
    node_t* head = create_and_init_node();
    counter_t* counter = create_and_init_counter();
    thread_args_t targs;

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

    // start our second thread
    pthread_t insert_thread;
    pthread_create(&insert_thread, NULL, insert_job, &targs);

    // do the same thing on our first thread
    insert_loop(targs.counter, targs.node, targs.limit);

    pthread_join(insert_thread, NULL);

    printf("Test 1 - final insert counter: %d", targs.counter->value);

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
    node_t* current_node = head;

    // set up our empty list
    for (int i = 0; i < LIMIT; i++) {
        current_node = push(current_node);
    }

    current_node = head;

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

    pthread_join(insert_thread, NULL);

    // we can risk printing the value directly here, as we should be done with 
    // both jobs
    printf("Test 2 - final insert counter: %d\n",  insert_targs.counter->value);
    printf("Test 2 - final get counter: %d\n",  get_targs.counter->value);

    return 0;
}

// Test three: "Starting with a list containing 1 million random integers, two threads running at the same time look up 1 million random integers each."
void test_three() {

    int num_get_threads = 2;

    node_t* head = create_and_init_node();

    counter_t* insert_counter = create_and_init_counter();
    counter_t* get_counter = create_and_init_counter();

    thread_args_t insert_targs;
    thread_args_t get_targs[num_get_threads];
    
    pthread_t get_threads[num_get_threads];

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
    
    // populate the list
    insert_job(&insert_targs);

    // set up the arguments to be passed to our "get" threads
    for (int i = 0; i < num_get_threads; i++ ) {
        get_targs[i].counter = get_counter;
        get_targs[i].node = current_node;
        get_targs[i].limit = LIMIT;
    }

    // start our threads
    for (int i = 0; i < num_get_threads; i++) {
        pthread_create(&get_threads[i], NULL, get_job, &get_targs[i]);
    }

    // join our threads once they are finished
    for (int i = 0; i < num_get_threads; i++) {
        pthread_join(get_threads[i], NULL);
    }

    // print our results
    for (int i = 0; i < num_get_threads; i++) {
        printf("Test 3 - final get%d counter: %d\n",  i + 1, get_targs[i].counter->value);
    }

    return 0;
}

int main() {
    srand(time(NULL));

    test_one();
    test_two();
    test_three();

    return 0;
}
