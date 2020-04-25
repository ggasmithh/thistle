/*
Author:     Garrett Smith
File:       hand_over_hand.c

Sources used:
    1)
https://github.com/angrave/SystemProgramming/wiki/Synchronization%2C-Part-1%3A-Mutex-Locks
        a) page 8 - Concurrent Linked Lists
        b) page 11 - Concurrent Queues
    2) manpages
    3) http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks-usage.pdf

*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#define LIMIT (1000000)
#define LIMIT (10)

typedef struct node {
  int data;
  struct node *next;
  pthread_mutex_t *lock;
} node_t;

typedef struct counter {
  int volatile value;
} counter_t;

typedef struct thread_args {
  node_t *node;
  counter_t *counter;
  int limit;
} thread_args_t;

counter_t *create_and_init_counter() {
  counter_t *c = (counter_t *)calloc(1, sizeof(counter_t));
  c->value = 0;
  return c;
}

node_t *create_and_init_node() {
  node_t *n = (node_t *)calloc(1, sizeof(node_t));
  n->lock = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(n->lock, NULL);
  return n;
}

void increment_counter(counter_t *c) { c->value++; }

int get_counter(counter_t *c) { return c->value; }

void decrement_counter(counter_t *c) { c->value--; }

// releases lock on current node, acquires lock on next node, returns pointer to
// next node
node_t *traverse(void *n) {
  if (n != NULL) {
    node_t *curr_node = (node_t *)n;
    if (curr_node->next != NULL) {
      node_t *next_node = (node_t *)curr_node->next;

      pthread_mutex_lock(&next_node->lock);
      pthread_mutex_unlock(&curr_node->lock);

      return next_node;
    }
  } else {
    return NULL;
  }
}

// returns the data for a node at a given address
int get_node_data(node_t *target_node) { return target_node->data; }

// pushes a new node to be the "next" of a given node
// (intend use: given node = tail of linked list)
// returns node that was pushed
node_t *push(void *n) {
  node_t *last_node = (node_t *)n;
  node_t *tmp = create_and_init_node();
  tmp->data = 0;
  tmp->next = NULL;
  last_node->next = tmp;
  return tmp;
}

void insert_data(int data, node_t *curr_node) { curr_node->data = data; }

void insert_loop(counter_t *counter, node_t *node, int limit) {
  int new_data;
  node_t *curr_node = node;
  while (get_counter(counter) < limit && curr_node != NULL) {
    new_data = (int)rand();
    insert_data(new_data, curr_node);
    curr_node = traverse(curr_node);
    increment_counter(counter);
  }
}

void lookup_loop(counter_t *counter, node_t *node, int limit) {
  int lookup_value;
  node_t *curr_node = node;
  while (get_counter(counter) < limit) {
    lookup_value = (int)rand();
    while (curr_node != NULL && get_node_data(curr_node) != lookup_value) {
      curr_node = traverse(curr_node);
    }
    increment_counter(counter);
  }
}

void insert_job(void *args) {
  thread_args_t *targs = (thread_args_t *)args;

  counter_t *counter = targs->counter;
  node_t *node = targs->node;
  int limit = targs->limit;

  insert_loop(counter, node, limit);
}

void lookup_job(void *args) {
  thread_args_t *targs = (thread_args_t *)args;

  counter_t *counter = targs->counter;
  node_t *node = targs->node;
  int limit = targs->limit;

  lookup_loop(counter, node, limit);
}

// Test one: "Starting with an empty list, two threads running at the same time
// insert 1 million random integers each on the same list."
void test_one() {
  node_t *head = create_and_init_node();
  counter_t *counter = create_and_init_counter();
  thread_args_t targs;

  // Used for traversal purposes. Points to the node that the program is
  // currently "looking at"
  node_t *current_node = head;

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

  printf("Test 1 - final insert counter: %d\n", targs.counter->value);
}

// Test two: "Starting with an empty list, one thread inserts 1 million random
// integers, while another thread looks up 1 million random integers at the same
// time."
void test_two() {
  node_t *head = create_and_init_node();

  counter_t *insert_counter = create_and_init_counter();
  counter_t *lookup_counter = create_and_init_counter();

  thread_args_t insert_targs;
  thread_args_t lookup_targs;

  // Used for traversal purposes. Points to the node that the program is
  // currently "looking at"
  node_t *current_node = head;

  // set up our empty list
  for (int i = 0; i < LIMIT; i++) {
    current_node = push(current_node);
  }

  current_node = head;

  insert_targs.counter = insert_counter;
  insert_targs.node = current_node;
  insert_targs.limit = LIMIT;

  lookup_targs.counter = lookup_counter;
  lookup_targs.node = current_node;
  lookup_targs.limit = LIMIT;

  // start our second thread
  pthread_t insert_thread;
  pthread_create(&insert_thread, NULL, insert_job, &insert_targs);

  // do the same thing on our first thread
  lookup_loop(lookup_targs.counter, lookup_targs.node, lookup_targs.limit);

  pthread_join(insert_thread, NULL);

  // we can risk printing the value directly here, as we should be done with
  // both jobs
  printf("Test 2 - final insert counter: %d\n", insert_targs.counter->value);
  printf("Test 2 - final lookup counter: %d\n", lookup_targs.counter->value);
}

// Test three: "Starting with a list containing 1 million random integers, two
// threads running at the same time look up 1 million random integers each."
void test_three() {

  int num_lookup_threads = 2;

  node_t *head = create_and_init_node();

  counter_t *insert_counter = create_and_init_counter();
  counter_t *lookup_counter = create_and_init_counter();

  thread_args_t insert_targs;
  thread_args_t lookup_targs[num_lookup_threads];

  pthread_t lookup_threads[num_lookup_threads];

  // Used for traversal purposes. Points to the node that the program is
  // currently "looking at"
  node_t *current_node = head;

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

  // set up the arguments to be passed to our "lookup" threads
  for (int i = 0; i < num_lookup_threads; i++) {
    lookup_targs[i].counter = lookup_counter;
    lookup_targs[i].node = current_node;
    lookup_targs[i].limit = LIMIT;
  }

  // start our threads
  for (int i = 0; i < num_lookup_threads; i++) {
    pthread_create(&lookup_threads[i], NULL, lookup_job, &lookup_targs[i]);
  }

  // join our threads once they are finished
  for (int i = 0; i < num_lookup_threads; i++) {
    pthread_join(lookup_threads[i], NULL);
  }

  // print our results
  for (int i = 0; i < num_lookup_threads; i++) {
    printf("Test 3 - final lookup%d counter: %d\n", i + 1,
           lookup_targs[i].counter->value);
  }
}

int main() {
  srand(time(NULL));

  test_one();
  test_two();
  test_three();

  return 0;
}
