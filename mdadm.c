#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

//16 disks combine them into 1 disk goes to 0 to 1 MB - 1.
//init op
uint32_t op;
//global var for mount/unmount
int mount = 0;

uint32_t encode_operation(jbod_cmd_t cmd, int disk_num, int block_num){
  //op will eventually be passed thru jbod operation
  //construct op by shifting the cmd field (starts at field 26)
  //cmd << 26
  //disk_num << 22
  //block_num doesnt need to be shifted (0-8)
  op = cmd << 26 | disk_num << 22 | block_num;
  return op;
}

int mdadm_mount(void) {

  if (mount == 0){
  jbod_operation(JBOD_MOUNT, NULL);
  mount = 1;
  return 1;
  }
  else {
    return -1;
  }

  //if called again, should retrun -1
  //keep global variable that whenever mount is called it goes to 1 
  //and whenever unmount is called should go to 0 and if mount called when
  //it is 1, fail and if unmount called when 0 fail
  //
  //if u have not yet called mount, every command should fail
  //can use same global varaible, fail if not = 1.
  //
  //invalid parameters 
  //must pass in a valid buffer. could be null if 0 length input
  //

}

int mdadm_unmount(void) {

//calling unmount on an already unmounted system should fail (-1)
if (mount == 1){
  jbod_operation(JBOD_UNMOUNT, NULL);
  mount = 0;
  return 1;
}
else {
  return -1;
}
}

/*
void translate_address(uint32_t linear_addr, int *disk_num, int *block_num, int *offset)
{
  //linear_addr = 10
  *disk_num = linear_addr / JBOD_DISK_SIZE;
  *block_num = ((linear_addr % JBOD_DISK_SIZE)) / JBOD_BLOCK_SIZE;
  *offset = (linear_addr % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;
}
int seek(int disk_num, int block_num) {
  assert(disk_num >= 0 && disk_num <= 15);
  assert(block_num >= 0 && block_num <= 15);
  encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0);
  //disk id and then once ur in the disk must seek to specific block
  encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num);
  //must make sure not passing invalid arguments
}
*/

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
  int disk_num, block_num, offset;
    //fail on an out-of-bound linear address
  //fail if it goes beyond the end of the linear address space
  if (mount == 0 || addr >= 1048576 || (addr + len) >= 1048576){
    return -1;
   }
    //fail when passed a NULL pointer and non-zero length
  if (buf == NULL && len != 0){
    return -1;
  }
    //fail on larger than 1024-byte I/O sizes
  if (len > 1024 || *buf > 1024){
    return -1;
  }
    //vars for address and length
  int length = len;
    //var to keep track amount of buffer read
  int buf_used = 0;

      // seek
    disk_num = addr / JBOD_DISK_SIZE;
    block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
    offset = (addr % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;
    //seek
    jbod_operation(encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
    jbod_operation(encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

      // read
      //make temp var
    uint8_t tmp[JBOD_BLOCK_SIZE];

      //while length is more than 0
    while (length > 0){
            //read current block u seek to, moves to next block immediately
      //implement cache 
      if (cache_lookup(disk_num, block_num, tmp) == -1){
        jbod_operation(encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
        jbod_operation(encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
        jbod_operation(encode_operation(JBOD_READ_BLOCK, 0, 0), tmp);
        cache_insert(disk_num, block_num, tmp);
      }
      //jbod_operation(encode_operation(JBOD_READ_BLOCK, 0, 0), tmp);
      //increment block num by 1
      block_num += 1;
      //IF - once reach end of disk, increment disk num, set block num to 0, seek to new disk
      if (block_num > 255){
        disk_num += 1;
        block_num = 0;
        jbod_operation(encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
        jbod_operation(encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
      }
      //outside of it
      //offset only matters first run of the loop
      //in loop after new seek to disk
      //if length + offset > JBOD_BLOCK_SIZE then amount of buffer used = JBOD_BLOCK_SIZE - offset
      if (length + offset > JBOD_BLOCK_SIZE){
        buf_used = JBOD_BLOCK_SIZE - offset;
      }
      //else { amount of buffer used = length}
      else {
        buf_used = length;
      }
      //memcpy(dest, src, size)//
      
      memcpy (buf + (len - length),tmp + offset, buf_used);
      //length -= amount of buffer used
      length -= buf_used;
      // process data
      //piazza post
      //seek to right disk and block
      //memcpy(buf + something, tmp, JBOD_BLOCK_SIZE);
        //set offset to 0 because once run once, offset no longer matters
      offset = 0;
    }
      //outside while return len
    return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
    //fail on an out-of-bound linear address
    //fail if it goes beyond the end of the linear address space
    if (mount == 0 || addr > 1048576 || (addr + len) > 1048576){
      return -1;
    }
    //fail when passed a NULL pointer and non-zero length
    if (buf == NULL && len != 0){
      return -1;
    }
    //fail on larger than 1024-byte I/O sizes
    if (len > 1024 || *buf > 1024){
      return -1;
    }
    //vars for disk num, block num, offset
    int disk_num, block_num, offset;
    //vars for address and length
    //int currentaddr = addr;
    int length = len;
    //var to keep track amount of buffer read
    int buf_used = 0;

    //while (currentaddr < addr + len){
      // seek
    disk_num = addr / JBOD_DISK_SIZE;
    block_num = addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
    offset = (addr % JBOD_DISK_SIZE) % JBOD_BLOCK_SIZE;
    jbod_operation(encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
    jbod_operation(encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

      //except in cases where you arent writing a full block, read block to tmp buf, 
      //and memcpy stuff from buf appropriately, then write tmp buf
      //when u do that, have to call seek to block and seek back to block you were just on
      //since read automatically moves to next block but dont want to move on yet
      //basically reversed read in the sense memcpy from buf to tmp then call write with tmp

      //while length is more than 0
    while (length > 0){

      //make temp buf
      uint8_t tmp[JBOD_BLOCK_SIZE];

      //implent cache
    
      if (cache_lookup(disk_num, block_num, tmp) == -1){
        jbod_operation(encode_operation(JBOD_READ_BLOCK, 0, 0), tmp);
        
      }

      //read block youo want t write over into temp
      //jbod_operation(encode_operation(JBOD_READ_BLOCK, 0, 0), tmp);
      //seek
      jbod_operation(encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
      jbod_operation(encode_operation(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);
      //if length + offset > JBOD_BLOCK_SIZE then amount of buffer used = JBOD_BLOCK_SIZE - offset
      if (length + offset > JBOD_BLOCK_SIZE){ 
          //set buf used to 256 - offset since that is how much would be already read
        buf_used = JBOD_BLOCK_SIZE - offset;
      }
      //else { amount of buffer used = length}
      else {
        buf_used = length;
      }
        //memcpy from buf to temp buf length being that of the amount of buf already used
      memcpy(tmp + offset, buf + (len-length), buf_used);

        //write to tmp
      jbod_operation(encode_operation(JBOD_WRITE_BLOCK, disk_num, block_num), tmp);

      //update cache 
      cache_update(disk_num, block_num, tmp);
      //increment block num by 1
      block_num += 1;
      //IF - once reach end of disk, increment disk num, set block num to 0, seek to new disk
      if (block_num > 255){
        disk_num += 1;
        block_num = 0;
        jbod_operation(encode_operation(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);
      }
      //set offset to 0
      offset = 0;
      //decrement temp length by amount of buf used
      length -= buf_used;
    }
//return len
    return len;
}