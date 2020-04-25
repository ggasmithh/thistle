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

#define LIMIT (1000000) // maximum length of linked list
#define WL3LUT                                                                 \
  (2) // number of LookUp threads for workload 3 (this should never be changed)

typedef struct node {
  int data;
  struct node *next;
  pthread_mutex_t *lock;
} node_t;

typedef struct counter {
  // we need the volatile keyword here because multiple threads will be reading
  // and writing to this value
  int volatile value;
} counter_t;

// this acts as a nice little package that we can pass to pthread_create(), and
// it allows us to associate separate counters to individual threads
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

// pushes a new node to be the "next" of a given node (intend use: given node =
// tail of linked list) returns node that was pushed
node_t *push(void *n) {
  node_t *last_node = (node_t *)n;
  node_t *tmp = create_and_init_node();
  tmp->data = 0;
  tmp->next = NULL;
  last_node->next = tmp;
  return tmp;
}

// the wrapper for insert_loop, intended to be used with pthread_create()
void insert_job(void *args) {
  thread_args_t *targs = (thread_args_t *)args;

  counter_t *counter = targs->counter;
  node_t *node = targs->node;
  int limit = targs->limit;

  int new_data;
  node_t *curr_node = node;
  while (get_counter(counter) < limit && curr_node != NULL) {
    new_data = (int)rand();
    curr_node->data = new_data;
    curr_node = traverse(curr_node);
    increment_counter(counter);
  }
}

// the wrapper for lookup_loop, intended to be used with pthread_create()
void lookup_job(void *args) {
  thread_args_t *targs = (thread_args_t *)args;

  counter_t *counter = targs->counter;
  node_t *node = targs->node;
  int limit = targs->limit;

  int lookup_value;
  node_t *curr_node = node;
  while (get_counter(counter) < limit) {
    lookup_value = (int)rand();
    while (curr_node != NULL && curr_node->data != lookup_value) {
      curr_node = traverse(curr_node);
    }
    increment_counter(counter);
  }
}

// Test one: "Starting with an empty list, two threads running at the same time
// insert 1 million random integers each on the same list."
void test_one() {
  clock_t start;
  clock_t end;

  node_t *head = create_and_init_node();
  thread_args_t targs;

  // Used for traversal purposes. Points to the node that the program is
  // currently "looking at"
  node_t *current_node = head;

  // set up our empty list
  for (int i = 0; i < LIMIT; i++) {
    current_node = push(current_node);
  }

  current_node = head;

  targs.counter = create_and_init_counter();
  targs.node = current_node;
  targs.limit = LIMIT;

  start = clock();

  // start our second thread
  pthread_t insert_thread;
  pthread_create(&insert_thread, NULL, insert_job, &targs);

  // do the same thing on our first thread
  insert_job(&targs);

  pthread_join(insert_thread, NULL);

  end = clock();

  printf("Workload 1 runtime: %.10e\n", (end - start) / (double)CLOCKS_PER_SEC);
}

// Test two: "Starting with an empty list, one thread inserts 1 million random
// integers, while another thread looks up 1 million random integers at the same
// time."
void test_two() {
  clock_t start;
  clock_t end;

  node_t *head = create_and_init_node();

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

  insert_targs.counter = create_and_init_counter();
  insert_targs.node = current_node;
  insert_targs.limit = LIMIT;

  lookup_targs.counter = create_and_init_counter();
  lookup_targs.node = current_node;
  lookup_targs.limit = LIMIT;

  start = clock();

  // start our second thread
  pthread_t insert_thread;
  pthread_create(&insert_thread, NULL, insert_job, &insert_targs);

  // do the same thing on our first thread
  lookup_job(&lookup_targs);

  pthread_join(insert_thread, NULL);

  end = clock();

  printf("Workload 2 runtime: %.10e\n", (end - start) / (double)CLOCKS_PER_SEC);
}

// Test three: "Starting with a list containing 1 million random integers, two
// threads running at the same time look up 1 million random integers each."
void test_three() {
  clock_t start;
  clock_t end;

  node_t *head = create_and_init_node();

  thread_args_t insert_targs;
  thread_args_t lookup_targs[WL3LUT];

  pthread_t lookup_thread;

  // Used for traversal purposes. Points to the node that the program is
  // currently "looking at"
  node_t *current_node = head;

  // set up our empty list
  for (int i = 0; i < LIMIT; i++) {
    current_node = push(current_node);
  }

  current_node = head;

  insert_targs.counter = create_and_init_counter();
  insert_targs.node = current_node;
  insert_targs.limit = LIMIT;

  // populate the list
  insert_job(&insert_targs);

  // set up the arguments to be passed to our "lookup" threads
  for (int i = 0; i < WL3LUT; i++) {
    lookup_targs[i].counter = create_and_init_counter();
    lookup_targs[i].node = current_node;
    lookup_targs[i].limit = LIMIT;
  }

  start = clock();

  // start second thread
  pthread_create(&lookup_thread, NULL, lookup_job, &lookup_targs[WL3LUT]);

  // do the same thing in our first thread
  lookup_job(&lookup_targs[0]);

  pthread_join(lookup_thread, NULL);

  end = clock();

  printf("Workload 3 runtime: %.10e\n", (end - start) / (double)CLOCKS_PER_SEC);
}

int main() {
  srand(time(NULL));

  test_one();
  test_two();
  test_three();

  return 0;
}
