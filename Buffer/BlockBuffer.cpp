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

  if(bufferNum!=E_BLOCKNOTINBUFFER){
    StaticBuffer::metainfo[bufferNum].timeStamp=0;
    for(int i=0;i<BUFFER_CAPACITY;i++){
      if(i!=bufferNum){
      if(StaticBuffer::metainfo[i].free==false){
        StaticBuffer::metainfo[i].timeStamp++;
      }
    } }
  }

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

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
      int status= loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.

        if(status!=SUCCESS){
          return status;
        }
        

    /* get the header of the block using the getHeader() function */
    HeadInfo head;
    getHeader(&head);

    // get number of attributes in the block.
    int noattrs=head.numAttrs;

    // get the number of slots in the block.
    int slots=head.numSlots;
    if(slotNum>=slots){
      return E_OUTOFBOUND;
    }

    // if input slotNum is not in the permitted range return E_OUTOFBOUND.

    /* offset bufferPtr to point to the beginning of the record at required
       slot. the block contains the header, the slotmap, followed by all
       the records. so, for example,
       record at slot x will be at bufferPtr + HEADER_SIZE + (x*recordSize)
       copy the record from `rec` to buffer using memcpy
       (hint: a record will be of size ATTR_SIZE * numAttrs)
    */
   memcpy(bufferPtr+HEADER_SIZE+head.numSlots+(slotNum*(16*noattrs)),rec,16*noattrs);

    // update dirty bit using setDirtyBit()
    StaticBuffer::setDirtyBit(this->blockNum);

    /* (the above function call should not fail since the block is already
       in buffer and the blockNum is valid. If the call does fail, there
       exists some other issue in the code) */


    return SUCCESS;
}
