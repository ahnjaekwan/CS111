#define _GNU_SOURCE
#include "SortedList.h"
#include <string.h>
#include <pthread.h>
#include <sched.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
  SortedListElement_t *current = list;
  if(current == NULL || element == NULL) //if corrupted
    return;
  if(current->next == NULL){ //if list is empty
    element->next = NULL;
    element->prev = current;
    current->next = element;
    return;
  } else{
    current = current->next; //move to the starting node
  }

  while(current->next != NULL) {
    if(strcmp(current->key, element->key) > 0) //find the right place where the specified element will be inserted in to
      break;
    current = current->next;
  } //go to the next node until it finds the place or reaches at the end
  
  if(opt_yield & INSERT_YIELD)
    sched_yield();

  if(strcmp(current->key, element->key) > 0){ //insert when found the right place to insert
    element->next = current;
    element->prev = current->prev;
    current->prev->next = element;
    current->prev = element;
  }
  else { //insert when loop reaches to the end
    element->next = current->next; //NULL
    element->prev = current;
    current->next = element;
  }
  return;
}

int SortedList_delete(SortedListElement_t *element) {
  //check if corrupted
  if(element == NULL)
    return 1;
  if(element->prev->next != element)
    return 1;
  if(element->next != NULL){
    if(element->next->prev != element)
      return 1;
  }

  if(opt_yield & DELETE_YIELD)
    sched_yield();

  if(element->prev != NULL) //delete by disconnecting
    element->prev->next = element->next;
  if(element->next != NULL) //delete by disconnecting
    element->next->prev = element->prev;
  return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  SortedListElement_t *current = list;
  if(current == NULL || key == NULL) //if corrupted
    return NULL;
  if(current->next == NULL) //if list is empty
    return NULL;
  else
    current = current->next; //move to the starting node

  while(current != NULL) {
    if(strcmp(current->key, key) == 0) //find the right place where the specified element should be located
      return current;
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();
    current = current->next;
  } //go to the next node until it finds the place or reaches at the end
  
  return NULL; //none is found
}

int SortedList_length(SortedList_t *list) {
  SortedListElement_t *current = list;
  if(current == NULL) //if head is corrupted
    return -1;

  int count = 0;
  while(current->next != NULL){
    count++; //count length
    if(opt_yield & LOOKUP_YIELD)
      sched_yield();
    current = current->next;
    if(current->prev->next != current) //if corrupted
      return -1;
  }
  return count;
}
