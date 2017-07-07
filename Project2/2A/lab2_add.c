/*
NAME: Jaekwan Ahn
EMAIL: ahnjk0513@gmail.com
ID: 604057669
*/

#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>

long long counter;
int num_iterations;
int num_threads;
int opt_yield;
int num_mode;
pthread_mutex_t pmutex;
int sync;

void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}
void add1(long long *pointer, long long value) {
  if(pthread_mutex_lock(&pmutex)){
    fprintf(stderr, "ERROR in pthread_mutex_lock()\n");
    exit(1);
  }
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  if(pthread_mutex_unlock(&pmutex)){
    fprintf(stderr, "ERROR in pthread_mutex_unlock()\n");
    exit(1);
  }
}
void add2(long long *pointer, long long value) {
  while(__sync_lock_test_and_set(&sync, 1));
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  __sync_lock_release(&sync);
}
void add3(long long *pointer, long long value) {
  long long newsum, oldval;
  //long long sum = *pointer + value;
  oldval = *pointer;
  newsum = oldval + value;
  if (opt_yield)
    sched_yield();
  //*pointer = sum;
  while(__sync_val_compare_and_swap(pointer, oldval, newsum) != oldval){
    oldval = *pointer;
    newsum = oldval + value;
    if (opt_yield)
      sched_yield();
  }
}
void add_mode(long long *pointer, long long value) {
  switch(num_mode) {
  case 0:
    add(pointer, value);
    break;
  case 1:
    add1(pointer, value);
    break;
  case 2:
    add2(pointer, value);
    break;
  case 3:
    add3(pointer, value);
    break;
  default:
    fprintf(stderr, "ERROR in wrong sync option\n");
  }
}
static void* addfunction(void *nothing) {
  int index;
  for(index = 0; index < num_iterations; index++) {
    //add 1 to the counter the specified number of times
    add_mode(&counter, 1);
    //add -1 to the coutner the specified number of times
    add_mode(&counter, -1);
  }
  //exit to re-join the parent thread
  pthread_exit(NULL);
  return NULL;
}
void *(*start_routine) (void *) = addfunction;
int main(int argc, char *argv[])
{
  struct timespec start,end;
  num_threads = 1;
  num_iterations = 1;
  opt_yield = 0;
  num_mode = 0;
  //takes parameters for the number of parallel threads and iterations
  while(1)
    {
            static const struct option long_options[] =
	      {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", no_argument, NULL, 'y'},
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
	      opt_yield = 1;
	      break;
	    case 's' :
	      if(optarg[0] == 'm'){
		num_mode = 1;
	      } else if(optarg[0] == 's'){
		num_mode = 2;
	      } else if(optarg[0] == 'c'){
		num_mode = 3;
	      }
	      break;
	    default:
	      fprintf(stderr, "ERROR, no such option\n");
	    }
    }
  //initializes a counter to zero
  counter = 0;
  //notes the starting time for the run
  clock_gettime(CLOCK_MONOTONIC, &start);
  //starts the specified number of threads
  pthread_t p[num_threads];
  int index;
  //addfunction for each thread
  for(index = 0; index < num_threads; index++) {
    if(pthread_create(&p[index], NULL, start_routine, (void *)(NULL))) {
      fprintf(stderr, "ERROR in pthread_create()\n");
      exit(1);
    }
  }
  //wait for all threads to complete
  for(index = 0; index < num_threads; index++) {
    if(pthread_join(p[index], NULL)){
      fprintf(stderr, "ERROR in pthread_join()\n");
      exit(1);
    }
  }
  //notes the ending time for the run
  clock_gettime(CLOCK_MONOTONIC, &end);
  //prints to stdout a CSV record
  long runtime = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
  int performed = num_threads * num_iterations * 2;
  char yield[2][7] = {"", "-yield"};
  char name[4][6] = {"-none", "-m", "-s", "-c"};
  printf("add%s%s,%d,%d,%d,%ld,%ld,%lld\n", yield[opt_yield], name[num_mode], num_threads, num_iterations, performed, runtime, runtime/performed, counter);
  //if any errors encountered
  if(counter != 0){
    exit(2);
  }
  //completes successfully
  exit(0);
  return 0;
}
  
