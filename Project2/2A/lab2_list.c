#include "SortedList.c"
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>

int num_threads = 1;
int num_iterations = 1;
int num_lists = 1;
int opt_yield = 0;
int opt_sync = 0;
int keylen = 10;
long total_waittime = 0;

SortedList_t *SList;
SortedListElement_t **Elements;
pthread_mutex_t *pmutex;
int *spin;


void sigHandler() { //catch a segmentation fault
  fprintf(stderr, "ERROR with segmentation fault\n");
  exit(2);
}

void free_all() { //free all memoried allocated
    free(SList);

    int index;
    for(index = 0; index < num_threads; index++){
      free(Elements[index]);
    }
  free(Elements);

    if(opt_sync == 1){
      for(index = 0; index < num_lists; index++){
        if(pthread_mutex_destroy(&pmutex[index])){
          fprintf(stderr, "ERROR in pthread_mutex_destroy()\n");
          exit(1);
        }
      }
      free(pmutex);
  }
  if(opt_sync == 2)
    free(spin);
  
  return;
}

char* generate_random_key() { //generate random keys in length of 10
  char *key = malloc(sizeof(char) * (keylen + 1));
  if(key == NULL)
      return NULL;
    
    int index;
    for(index = 0; index < keylen; index++){
      key[index] = (char) (rand() % 255 + 1);
    }
    key[keylen] = '\0';
    
    return key;
}

int hash_function(const char *key) { //generate hash number
  int hashnum = 0;
  int index;
  for(index = 0; index < keylen; index++){
    hashnum += key[index] + 35 - index; //random
  }
  hashnum = hashnum % num_lists;
  if(hashnum < 0){ //make sure that hash number is not negative
    hashnum = hashnum * (-1);
  }
  
  return hashnum;
}

void sync_lock(int index) { //lock depending on sync option
    switch(opt_sync){
    case 0:
      break;
    case 1:
      //pthread_mutexes
      pthread_mutex_lock(&pmutex[index]);
      break;
    case 2:
      //spin locks
      while(__sync_lock_test_and_set(&spin[index], 1));
      break;
    default:
      fprintf(stderr, "ERROR in wrong sync option\n");
      exit(1);
    }
}

void sync_unlock(int index) { //unlock depending on sync option
    switch(opt_sync){
    case 0:
      break;
    case 1:
      //pthread_mutexes
      pthread_mutex_unlock(&pmutex[index]);
      break;
    case 2:
      //spin locks
      __sync_lock_release(&spin[index]);
      break;
    default:
      fprintf(stderr, "ERROR in wrong sync option\n");
      exit(1);
    }
}

void* execute(void *thread_index1) { //execution function for each thread
    //starts with a set of pre-allocated and initialized elements
    int thread_index = *((int *) thread_index1);
    struct timespec waitstart, waitend;
    int index, hashnum;
    long waittime;
  
    //inserts them all into a list
    for(index = 0; index < num_iterations; index++) {
      hashnum = hash_function(Elements[thread_index][index].key);
      if(opt_sync > 0){
        clock_gettime(CLOCK_MONOTONIC, &waitstart);
        sync_lock(hashnum);
        clock_gettime(CLOCK_MONOTONIC, &waitend);
        waittime = (waitend.tv_sec - waitstart.tv_sec) * 1000000000L + (waitend.tv_nsec - waitstart.tv_nsec);
        total_waittime += waittime;
    }
      SortedList_insert(&SList[hashnum], &Elements[thread_index][index]);
      if(opt_sync > 0)
        sync_unlock(hashnum);
    }
  
    //gets the list length
    int listlength = 0;
    int singlelength;
    if(opt_sync > 0){
      for(index = 0; index < num_lists; index++){
        clock_gettime(CLOCK_MONOTONIC, &waitstart);
        sync_lock(index);
        clock_gettime(CLOCK_MONOTONIC, &waitend);
        waittime = (waitend.tv_sec - waitstart.tv_sec) * 1000000000L + (waitend.tv_nsec - waitstart.tv_nsec);
      total_waittime += waittime;
    }
    }
  for(index = 0; index < num_lists; index++){
    singlelength = SortedList_length(&SList[index]);
      if(singlelength == -1){
        fprintf(stderr, "ERROR in length of list: list is corrupted1\n");
        exit(2);
    }
    listlength += singlelength;
  }
  if(opt_sync > 0) {
      for(index = 0; index < num_lists; index++){
        sync_unlock(index);
      }
    }

    //looks up and deletes each of the keys it had previously inserted
    SortedListElement_t *deleted;
    for(index = 0; index < num_iterations; index++) {
      hashnum = hash_function(Elements[thread_index][index].key);
      if(opt_sync > 0) {
        clock_gettime(CLOCK_MONOTONIC, &waitstart);
        sync_lock(hashnum);
        clock_gettime(CLOCK_MONOTONIC, &waitend);
        waittime = (waitend.tv_sec - waitstart.tv_sec) * 1000000000L + (waitend.tv_nsec - waitstart.tv_nsec);
        total_waittime += waittime;
      }
      deleted = SortedList_lookup(&SList[hashnum], Elements[thread_index][index].key);
      if(deleted == NULL){
        fprintf(stderr, "ERROR in SortedList_lookup()\n");
        exit(1);
      }
      if(SortedList_delete(deleted) == 1){
        fprintf(stderr, "ERROR in SortedList_delete()\n");
        exit(1);
      }
      if(opt_sync > 0)
        sync_unlock(hashnum);
    }

    //exits to re-join the parent thread
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[])
{
    struct timespec start, end;
    int num_index, index, ch;
    char total_yield[4];

    //takes parameters for the number of parallel threads and the number of iterations
    //and a parameter to enable optional critical section yields
    static const struct option long_options[] =
    {
      {"threads", required_argument, NULL, 't'},
      {"iterations", required_argument, NULL, 'i'},
      {"yield", required_argument, NULL, 'y'},
      {"sync", required_argument, NULL, 's'},
      {"lists", required_argument, NULL, 'l'},
      {0,0,0,0}
    };

    while(1){
      ch = getopt_long(argc, argv, "",long_options, NULL);
      if(ch == -1) {
        break;
      }
      switch(ch) {
      case 't' :
        num_threads = atoi(optarg);
        if(num_threads <= 0){
          fprintf(stderr, "ERROR in thread number\n");
          exit(1);
        }
        break;
      case 'i' :
        num_iterations = atoi(optarg);
        if(num_iterations <= 0){
          fprintf(stderr, "ERROR in iteration number\n");
          exit(1);
        }
        break;
      case 'y' :
        num_index = (int) strlen(optarg);
        if(num_index > 3){
          fprintf(stderr, "ERROR in --yield option\n");
          exit(1);
        } 
        for(index = 0; index < num_index; index++){
            if(optarg[0] == 'i'){
          opt_yield |= INSERT_YIELD;
            } else if(optarg[0] == 'd'){
          opt_yield |= DELETE_YIELD;
            } else if(optarg[0] == 'l'){
          opt_yield |= LOOKUP_YIELD;
            } else{
            fprintf(stderr, "ERROR in --yield option\n");
              exit(1);
            }
        }
        strcpy(total_yield, optarg);
        break;
      case 's':
        num_index = (int) strlen(optarg);
        if(num_index != 1){
          fprintf(stderr, "ERROR in --sync option\n");
          exit(1);
        } 
        if(optarg[0] == 'm'){
            opt_sync = 1;
        } else if(optarg[0] == 's'){
          opt_sync = 2;
        } else{
          fprintf(stderr, "ERROR in --sync option\n");
          exit(1);
        }
        break;
      case 'l':
        num_lists = atoi(optarg);
        if(num_lists <= 0){
          fprintf(stderr, "ERROR in list number\n");
          exit(1);
        }
        break;
      default:
        fprintf(stderr, "ERROR, no such option\n");
        exit(1);
      }
    }

    //catch a signal when seg fault
    signal(SIGSEGV, sigHandler);

    //initialize mutex
    if(opt_sync == 1){
      pmutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t) * num_lists);
      if(pmutex == NULL){
        fprintf(stderr, "ERROR in malloc pthread_mutex_t\n");
      exit(1);
      }
      for(index = 0; index < num_lists; index++){
        if(pthread_mutex_init(&pmutex[index],NULL)){
          fprintf(stderr, "ERROR in pthread_mutex_init()\n");
          exit(1);
        } 
      }
    }

    //initialize spin-lock
    if(opt_sync == 2){
      spin = (int *) malloc(sizeof(int) * num_lists);
      if(spin == NULL){
        fprintf(stderr, "ERROR in malloc spin-lock\n");
      exit(1);
      }
      for(index = 0; index < num_lists; index++){
        spin[index] = 0;
      }
    }

    //initialize an empty list
    SList = (SortedList_t *) malloc(sizeof(SortedList_t) * num_lists);
    if(SList == NULL) {
      fprintf(stderr, "ERROR in malloc SortedList_t\n");
    exit(1);
    }
    for(index = 0; index < num_lists; index++){
      SList[index].prev = NULL;
      SList[index].next = NULL;
      SList[index].key = NULL;
    }

    //creates and initializes the required number of list elements with random keys
    int index2;
    srand(time(NULL));
    Elements = (SortedListElement_t**) malloc(sizeof(SortedListElement_t*) * num_threads);
    if(Elements == NULL){
      fprintf(stderr, "ERROR in malloc SortedListElement_t**\n");
    exit(1);
    }
    for(index = 0; index < num_threads; index++){
      Elements[index] = (SortedListElement_t *) malloc(sizeof(SortedListElement_t) * num_iterations);
      if(Elements[index] == NULL){
        fprintf(stderr, "ERROR in malloc SortedListElement_t*\n");
      exit(1);
      }
      for(index2 = 0; index2 < num_iterations; index2++){
      Elements[index][index2].key = generate_random_key();
      }
    }

    //starts the specified number of threads
    pthread_t p[num_threads];
    int threads_id[num_threads];

    //notes the starting time for the run
    if(clock_gettime(CLOCK_MONOTONIC, &start)){
    fprintf(stderr, "ERROR in clock_gettime()\n");
    exit(1);
    }

    //execute each threads
    for(index = 0; index < num_threads; index++){
      //start
      threads_id[index] = index;
      if(pthread_create(&p[index], NULL, execute, &threads_id[index])) {
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
    if(clock_gettime(CLOCK_MONOTONIC, &end)){
    fprintf(stderr, "ERROR in clock_gettime()\n");
      exit(1);
    }

    //checks the length of the list to confirm that it is zero
    int length = 0;
    int total_length = 0;
    for(index = 0; index < num_lists; index++){
      length = SortedList_length(&SList[index]);
      if(length == -1){
        fprintf(stderr, "EERROR in length of list: list is corrupted\n");
        exit(2);
      }
      total_length += length;
    }
    if(total_length != 0) {
      fprintf(stderr, "ERROR in length of list: %d\n", total_length);
      exit(2);
    }

    //free elements
    free_all();

    //prints to stdout a CSV record
    long runtime = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    int performed = num_threads * num_iterations * 3;
    char *yieldopts;// = {"none", "i", "d", "l", "id", "il", "dl", "idl"};
    if(opt_yield == 0)
      yieldopts = "none";
    else
      yieldopts = total_yield;
    char syncopts[3][5] = {"none", "m", "s"};
    printf("list-%s-%s,%d,%d,%d,%d,%ld,%ld,%ld\n", yieldopts, syncopts[opt_sync], 
      num_threads, num_iterations, num_lists, performed, runtime, runtime/performed);
  
    exit(0);
    return 0;
}
