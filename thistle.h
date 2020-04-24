#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

//#define LIMIT (1000000)
#define LIMIT (10)

typedef struct node {
    int data;
    struct node* next;
    pthread_mutex_t lock;
} node_t;

typedef struct counter {
    int volatile value;
} counter_t;

typedef struct thread_args {
    node_t* node;
    counter_t* counter;
    int limit;
} thread_args_t;
