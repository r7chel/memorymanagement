#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

//global var for cache created
int created = 0;
//global var for empty line in cache
int lineempty = -1;
//global var for prev line
int prevline = -1;

int cache_create(int num_entries) {
  //if num entries out of bound - fail
  if (num_entries < 2 || num_entries > 4096){
    return -1;
  }
  //if cache already created - fail
  if (created == 1){
    return -1;
  }
  //allocate memory for cache
  cache = calloc(num_entries, sizeof(cache_entry_t));
  //set cache size to num entries
  cache_size = num_entries;
  //set created var to 1 since cache has been created
  created = 1;
  //set clock to 0
  clock = 0;
  int i = 0;
  //make invalid
  while (i < cache_size){
    cache[i].valid = false;
    cache[i].access_time = 99999;
    i += 1;
  }
  //return 1
  return 1;
}

int cache_destroy(void) {
  //if cache not already created - fail
  if (created == 0){
    return -1;
  }
  //get rid of pointers to allocated mem
  free(cache);
  //clear cache dangling pointers 
  cache = NULL;
  //set cache size to 0
  cache_size = 0;
  //set create to 0 since cache is no longer made
  created = 0;
  //return 1
  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) 
{
  //if buf is null or cache has not already been created - fail
  if (buf == NULL || created == 0) 
  {
    return -1;
  }
  //if disk or block num out of bounds - fail
  if (disk_num >= 0 && disk_num < 16 && block_num < 256 && block_num >= 0)
  {
 
    //var for number of empty lines in cache
    int numempty = 0;
    for (int i = 0; i < cache_size; i++)
    {
      if (cache[i].valid == false)
      {
        //increment number of empty lines
        numempty += 1;
      }
    }
    //if the size of the cache is not equal to nuber of empty lines
    if (cache_size != numempty)
    {
      //increment num queries
      num_queries += 1;
      //int i = 0;
      for(int i = 0;i < cache_size; i++)
      {
      //check if the disk and block nums match
      if(cache[i].disk_num == disk_num && cache[i].block_num == block_num)
      {
        //copy block into buf, size of block size
        memcpy(buf, cache[i].block, 256);
        //increment number of hits
        num_hits += 1;
        //increment clock
        clock += 1; //here testing
        //set access time to clock
        cache[i].access_time = clock;
        //printf("\nLOOK: (%d, %d), (%d, %d), (%d, %d)\n", cache[0].disk_num, cache[0].access_time, cache[1].disk_num, cache[1].access_time, cache[2].disk_num, cache[2].access_time);
        return 1;
      }

      //printf("\nLOOK: (%d, %d), (%d, %d), (%d, %d)\n", cache[0].disk_num, cache[0].access_time, cache[1].disk_num, cache[1].access_time, cache[2].disk_num, cache[2].access_time);
      //return 1
      //return 1;
      }
    }
   // printf("cache empty");
  }
 // printf("%d %d %d\n", cache[0].disk_num, cache[1].disk_num, cache[2].disk_num);
  //return 1
  return -1;
}
//return -1;


void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  int i = 0;
  while (i < cache_size){
    //if disk and block num match that of the catch
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      //update cache by copying buf to block, size of block size
      memcpy(cache[i].block, buf, 256);
      clock += 1;//increment clock
      cache[i].access_time = clock;//set access time to clock
    }
    //increment i
    i += 1;
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  //if buf is null or cache has not already been created - fail
  if (buf == NULL || created == 0 || cache == NULL) {
    return -1;
  }
  //var for empty spot and previous time
  int emptyspot = 0;
  //set to large number
  int prevtime = 1000000;
    //if disk or block num out of bounds - fail
  if (disk_num >= 0 && disk_num < 16 && block_num < 256 && block_num >= 0){
    //if lookup fails, want to insert into cache
    if (cache_lookup(disk_num, block_num, buf) == -1)
    {
      //for i less than cache size
      for(int i = 0;i < cache_size; i++)
      {
        //if access time is less than the previous time
        if (cache[i].access_time < prevtime){
          //set previous time to access time 
          prevtime = cache[i].access_time;
        }
      }
      //for i less than cache size
      for(int i = 0; i < cache_size; i++){
        //printf("%d", k);
        //if cache i is empty set empty check to true
        if (cache[i].valid == false){
         // printf("%d", k);
         //set line that is empty to i
          lineempty = i;
          //set empty checker to 1
          emptyspot = 1;
          break;
        }
        //if access time equals previoous time
        if (cache[i].access_time == prevtime){
          //set previous line to i
          prevline = i;
        }
      }

      //if empty check is true
      if (emptyspot == 1){
       // printf("filling on empty %d", lineempty);
       //copy buf to block of empty line, size of block size
        memcpy(cache[lineempty].block, buf, 256);
        //cache spot is now filled so set valid to true
        cache[lineempty].valid = true;
        //match up disk and block nums
        cache[lineempty].disk_num = disk_num;
        //increment clock and set access time to it
        clock += 1;
        cache[lineempty].access_time = clock;
        cache[lineempty].block_num = block_num;
      }

      //if empty checker is false and spot is filled
      if (emptyspot == 0){ 
       // printf("filling on lru");
       //copy buf to previous line instead of filled line, size of block num
        memcpy(cache[prevline].block, buf, 256);
        //set previous line valid to true bc it is now filled
        cache[prevline].valid = true;
        //match up disk and block nums
        cache[prevline].disk_num = disk_num;
        //increment clock and set accesstime to clock
        clock += 1;
        cache[prevline].access_time = clock;
        cache[prevline].block_num = block_num;
      }
     //printf("\nINSERT: (%d, %d), (%d, %d), (%d, %d)\n", cache[0].disk_num, cache[0].access_time, cache[1].disk_num, cache[1].access_time, cache[2].disk_num, cache[2].access_time);
        //return 1
        return 1;
        
    }
  
  }
//return -1
return -1;
}

bool cache_enabled(void) {
  return false;
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}