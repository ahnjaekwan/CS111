/*
NAME: Jaekwan Ahn
EMAIL: ahnjk0513@gmail.com
ID: 604057669
*/

#include "SortedList.c"
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <string.h>

int num_threads;
int num_iterations;
int opt_yield;
int opt_sync;
SortedList_t SList;
SortedListElement_t *Elements;
char **randomKeys;
int keylen = 10;
pthread_mutex_t pmutex;
int spin;

void generate_random_key(char *key) {
  key = malloc(sizeof(char) * (keylen + 1));
  if(key == NULL){
    fprintf(stderr, "ERROR in malloc key\n");
    exit(1);
  }
  int index;
  for(index = 0; index < keylen; index++){
    key[index] = (char) (rand() % 255 + 1);
  }
  key[keylen] = '\0';
}
static void sync_lock() {
  switch(opt_sync){
  case 0:
    break;
  case 1:
    //pthread_mutexes
    pthread_mutex_lock(&pmutex);
    break;
  case 2:
    //spin locks
    while(__sync_lock_test_and_set(&spin, 1));
    break;
  default:
    fprintf(stderr, "ERROR in wrong sync option\n");
    exit(2);
  }
}
static void sync_unlock() {
  switch(opt_sync){
  case 0:
    break;
  case 1:
    //pthread_mutexes
    pthread_mutex_unlock(&pmutex);
    break;
  case 2:
    //spin locks
    __sync_lock_release(&spin);
    break;
  default:
    fprintf(stderr, "ERROR in wrong sync option\n");
    exit(2);
  }
}
static void* execute(void *off) {
  //starts with a set of pre-allocated and initialized elements
  int some = *((int *) off);
  int startIt = some * num_iterations;
  int until = startIt + num_iterations - 1;
  
  //inserts them all into a list
  int index;
  for(index = startIt; index < until; index++) {
    sync_lock();
    SortedList_insert(&SList, &Elements[index]);
    sync_unlock();
  }
  
  //gets the list length
  sync_lock();
  int listlen = SortedList_length(&SList);
  sync_unlock();

  //looks up and deletes each of the keys it had previously inserted
  for(index = startIt; index < until; index++) {
    sync_lock();
    SortedList_delete(SortedList_lookup(&SList, randomKeys[index]));
    sync_unlock();
  }
  
  //exits to re-join the parent thread
  pthread_exit(NULL);
  return NULL;
}

void *(*start_routine) (void *) = execute;

int main(int argc, char *argv[])
{
  struct timespec start,end;
  num_threads = 1;
  num_iterations = 1;
  opt_yield = 0;
  opt_sync = 0;
  int num_index, index;

  //takes parameters for the number of parallel threads and the number of iterations
  //and a parameter to enable optional critical section yields
  while(1)
    {
          static const struct option long_options[] =
	    {
	      {"threads", required_argument, NULL, 't'},
	      {"iterations", required_argument, NULL, 'i'},
	      {"yield", required_argument, NULL, 'y'},
	      {"sync", required_argument, NULL, 's'},
	      {0,0,0,0}
	    };
	  int option_index = 0;
	  int ch;
	  ch = getopt_long(argc, argv, "",long_options, &option_index);
	  if(ch == -1) {
	    break;
	  }
	  switch(ch) {
	  case 't' :
	    num_threads = atoi(optarg);
	    break;
	  case 'i' :
	    num_iterations = atoi(optarg);
	    break;
	  case 'y' :
	    num_index = (int) strlen(optarg);
	    for(index = 0; index < num_index; index++){
	      if(optarg[0] == 'i'){
		opt_yield|= INSERT_YIELD;
	      }else if(optarg[0] == 'd'){
		opt_yield |= DELETE_YIELD;
	      }else if(optarg[0] == 'l'){
		opt_yield |= LOOKUP_YIELD;
	      }
	    }
	    break;
	  case 's':
	    if(optarg[0] == 'm'){
	      opt_sync = 1;
	    }else if(optarg[0] == 's'){
	      opt_sync = 2;
	    }
	    break;
	  default:
	    fprintf(stderr, "ERROR, no such option\n");
	  }
    }
  
  //initialize an empty list
  SList.prev = NULL;
  SList.next = NULL;
  SList.key = NULL;

  //creates and initializes the required number of list elements
  int num_required = num_threads * num_iterations;
  SortedListElement_t *created = (SortedListElement_t *) malloc(sizeof(SortedListElement_t) * num_required);
  if(created == NULL) {
    fprintf(stderr, "ERROR in malloc SortedListElement_t\n");
    free(created);
    exit(1);
  }
  randomKeys = (char **) malloc(sizeof(char *) * num_required);
  if(randomKeys == NULL) {
    fprintf(stderr, "ERROR in malloc random keys\n");
    free(randomKeys);
    exit(1);
  }
  for(index = 0; index < num_required; index++){
    generate_random_key(randomKeys[index]);
    created[index].key = randomKeys[index];
  }
  Elements = created;

  //notes the starting time for the run
  clock_gettime(CLOCK_MONOTONIC, &start);

  //starts the specified number of threads
  pthread_t p[num_threads];

  //execute each threads
  for(index = 0; index < num_threads; index++){
    //start
    if(pthread_create(&p[index], NULL, start_routine, (void *)(NULL))) {
      fprintf(stderr, "ERROR in pthread_create()\n");
      exit(1);
    }
  }
  
  //waits for all threads to complete
  for(index = 0; index < num_threads; index++) {
    if(pthread_join(p[index], NULL)){
      fprintf(stderr, "ERROR in pthread_join()\n");
      exit(1);
    }
  }
  
  //notes the ending time
  clock_gettime(CLOCK_MONOTONIC, &end);

  //checks the length of the list to confirm that it is zero
  int check = SortedList_length(&SList);
  if(check != 0) {
    fprintf(stderr, "ERROR in length of list");
    exit(2);
  }
  
  //prints to stdout a CSV record
  long runtime = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
  int performed = num_threads * num_iterations * 3;
  char yieldopts[8][6] = {"-none", "-i", "-d", "-l", "-id", "-il", "-dl", "-idl"};
  char syncopts[3][6] = {"-none", "-m", "-s"};
  printf("list%s%s,%d,%d,1,%d,%ld,%ld\n", yieldopts[opt_yield], syncopts[opt_sync], num_threads, num_iterations, performed, runtime, runtime/performed);

  //free elements at the end of the test
  free(Elements);
  for(index = 0; index < num_required; index++){
    free(randomKeys[index]);
  }
  free(randomKeys);
  
  exit(0);
  return 0;
}
