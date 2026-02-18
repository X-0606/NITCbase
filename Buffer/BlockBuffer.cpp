#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>
#include "../Disk_Class/Disk.h"


BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum=blockNum;
}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *buffer;

  int retu=loadBlockAndGetBufferPtr(&buffer);

  if(retu!=SUCCESS){
    return retu;
  }

  memcpy(&head->numSlots, buffer + 24, 4);
  memcpy(&head->numEntries,buffer+16, 4);
  memcpy(&head->numAttrs, buffer+20, 4);
  memcpy(&head->rblock,buffer+12, 4);
  memcpy(&head->lblock, buffer+8, 4);

  return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;
  this->getHeader(&head);
  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  unsigned char* buffer;

 int retu=loadBlockAndGetBufferPtr(&buffer);

  if(retu!=SUCCESS){
    return retu;
  }
  

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = buffer+32+head.numSlots+slotNum*recordSize;

  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}


int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}



/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;

  getHeader(&head);
  // get the header of the block using getHeader() function

  int slotCount = head.numSlots;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
  
  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap,slotMapInBuffer,slotCount);

  return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

    double diff;
    if (attrType == STRING)
        diff = strcmp(attr1.sVal, attr2.sVal);
    else if(attrType==NUMBER){
      diff=attr1.nVal-attr2.nVal;
    }

    if(diff>0){
      return 1;
    }
    else if(diff<0){
      return -1;
    }
    else{
      return 0;
    }

    /*
    if diff > 0 then return 1
    if diff < 0 then return -1
    if diff = 0 then return 0
    */
}
