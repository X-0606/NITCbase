#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "../Disk_Class/Disk.h"

int count=0;

BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum=blockNum;
}

BlockBuffer::BlockBuffer(char blockType) {
    // allocate a block on the disk and a buffer in memory to hold the new block of
    // given type using getFreeBlock function and get the return error codes if any.
    int blockTypeInt;
    if (blockType == 'R') blockTypeInt = REC;
    else if (blockType == 'I') blockTypeInt = IND_INTERNAL;
    else if (blockType == 'L') blockTypeInt = IND_LEAF;

    int allocatedblk = BlockBuffer::getFreeBlock(blockTypeInt);

    // set the blockNum field of the object to that of the allocated block
    // number if the method returned a valid block number,
    // otherwise set the error code returned as the block number.
    this->blockNum = allocatedblk;

  
}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

RecBuffer::RecBuffer() : BlockBuffer('R'){}

IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType){}
// call the corresponding parent constructor
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum){}
IndInternal::IndInternal() : IndBuffer('I') {}
// call the corresponding parent constructor
// 'I' used to denote IndInternal.

IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum) {}
// call the corresponding parent constructor
IndLeaf::IndLeaf() : IndBuffer('L'){} // this is the way to call parent non-default constructor.
                      // 'L' used to denote IndLeaf.

                      //this is the way to call parent non-default constructor.
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum){}

int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *buffer;

  int retu=loadBlockAndGetBufferPtr(&buffer);

  if(retu!=SUCCESS){
    return retu;
  }
  memcpy(&head->blockType,buffer + 0,  4);
  memcpy(&head->pblock, buffer + 4,  4);  
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
  unsigned char *slotPointer = buffer+HEADER_SIZE+head.numSlots+slotNum*recordSize;

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

    count++;

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

   memcpy(bufferPtr+HEADER_SIZE+head.numSlots+(slotNum*(ATTR_SIZE*noattrs)),rec,16*noattrs);


    // update dirty bit using setDirtyBit()
    StaticBuffer::setDirtyBit(this->blockNum);

    /* (the above function call should not fail since the block is already
       in buffer and the blockNum is valid. If the call does fail, there
       exists some other issue in the code) */


    return SUCCESS;
}


int BlockBuffer::setHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    // loadBlockAndGetBufferPtr(&bufferPtr).
    int s=loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if(s!=SUCCESS){
      return s;
    }

    // cast bufferPtr to type HeadInfo*
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    //(hint: bufferHeader->numSlots = head->numSlots )
    bufferHeader->lblock=head->lblock;
    bufferHeader->numAttrs=head->numAttrs;
    bufferHeader->numEntries=head->numEntries;
    bufferHeader->numSlots=head->numSlots;
    bufferHeader->pblock=head->pblock;
    bufferHeader->rblock=head->rblock;
    bufferHeader->blockType=head->blockType;

    s=StaticBuffer::setDirtyBit(this->blockNum);
    if(s!=SUCCESS){
      return s;
    }

    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed, return the error code

    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    // loadBlockAndGetBufferPtr(&bufferPtr).
    int s=loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if(s!=SUCCESS){
      return s;
    }

    // store the input block type in the first 4 bytes of the buffer.
    // (hint: cast bufferPtr to int32_t* and then assign it)
    *((int32_t *)bufferPtr) = blockType;
     
    // update the StaticBuffer::blockAllocMap entry corresponding to the
    // object's block number to `blockType`.
    StaticBuffer::blockAllocMap[this->blockNum]=(unsigned char)blockType;
   
   s=StaticBuffer::setDirtyBit(this->blockNum);
    if(s!=SUCCESS){
      return s;
    }
    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed, return the error code

    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType){

    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    int blk=-1;
    for(int i=0;i<8192;i++){
      if(StaticBuffer::blockAllocMap[i]==UNUSED_BLK){
        blk=i;
        break;
      }
    }
    if(blk==-1){
    return E_DISKFULL;
    }
    

    // if no block is free, return E_DISKFULL.

    this->blockNum=blk;

    // set the object's blockNum to the block number of the free block.
    int allocatedbuffernum=StaticBuffer::getFreeBuffer(blk);
    
    // find a free buffer using StaticBuffer::getFreeBuffer() .

    // initialize the header of the block passing a struct HeadInfo with values
    // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
    // to the setHeader() function.
    HeadInfo head;
    head.blockType = (int32_t)blockType;
    head.pblock=-1;
    head.lblock=-1;
    head.rblock=-1;
    head.numSlots=0;
    head.numAttrs=0;
    head.numEntries=0;

    this->setHeader(&head);

    this->setBlockType(blockType);


    // update the block type of the block to the input block type using setBlockType().

    // return block number of the free block.
    return blk;
}


int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
   
       int s=loadBlockAndGetBufferPtr(&bufferPtr);
       
    if (s != SUCCESS){
        return s;
    }
       
    HeadInfo head;
    getHeader(&head);
    // get the header of the block using the getHeader() function

    int numSlots = head.numSlots;

    // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
    // argument `slotMap` to the buffer replacing the existing slotmap.
    // Note that size of slotmap is `numSlots`
    memcpy(bufferPtr+HEADER_SIZE,slotMap,numSlots);

    // update dirty bit using StaticBuffer::setDirtyBit
    // if setDirtyBit failed, return the value returned by the call
    s=StaticBuffer::setDirtyBit(this->blockNum);

    return SUCCESS;
}


void BlockBuffer::releaseBlock() {
    // if blockNum is INVALID_BLOCKNUM (-1), or it is invalidated already, do nothing
    if (this->blockNum == INVALID_BLOCKNUM) {
        return;
    }

    // else
    /* get the buffer number of the buffer assigned to the block
       using StaticBuffer::getBufferNum().
       (this function return E_BLOCKNOTINBUFFER if the block is not
       currently loaded in the buffer)
    */
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    // if the block is present in the buffer, free the buffer
    // by setting the free flag of its StaticBuffer::tableMetaInfo entry
    // to true.
    if (bufferNum != E_BLOCKNOTINBUFFER) {
        StaticBuffer::metainfo[bufferNum].free = true;
    }

    // free the block in disk by setting the data type of the entry
    // corresponding to the block number in StaticBuffer::blockAllocMap
    // to UNUSED_BLK.
    StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;

    // set the object's blockNum to INVALID_BLOCK (-1)
    this->blockNum = INVALID_BLOCKNUM;
}

int BlockBuffer::getBlockNum(){

    return this->blockNum;
}

int IndInternal::getEntry(void *ptr, int indexNum) 
{
  // if the indexNum is not in the valid range of [0, MAX_KEYS_INTERNAL-1]
  //     return E_OUTOFBOUND.
  if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL)
  {
    return E_OUTOFBOUND;
  }

  unsigned char *bufferPtr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
     int ret=loadBlockAndGetBufferPtr(&bufferPtr);


  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  //     return the value returned by the call.
  if(ret!=SUCCESS)
  {
    return ret;
  }

  // typecast the void pointer to an internal entry pointer
  struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

  /*
  - copy the entries from the indexNum`th entry to *internalEntry
  - make sure that each field is copied individually as in the following code
  - the lChild and rChild fields of InternalEntry are of type int32_t
  - int32_t is a type of int that is guaranteed to be 4 bytes across every
    C++ implementation. sizeof(int32_t) = 4
  */
  

  /* the indexNum'th entry will begin at an offset of
     HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
     from bufferPtr */
  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

  memcpy(&(internalEntry->lChild), entryPtr, sizeof(int32_t));
  memcpy(&(internalEntry->attrVal), entryPtr + 4, sizeof(Attribute));
  memcpy(&(internalEntry->rChild), entryPtr + 20, 4);

  // return SUCCESS.
  return SUCCESS;
}
int IndLeaf::getEntry(void *ptr, int indexNum) 
{

  // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
  //     return E_OUTOFBOUND.
  if (indexNum < 0 || indexNum >= MAX_KEYS_LEAF)
  {
    return E_OUTOFBOUND;
  }

  unsigned char *bufferPtr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  //     return the value returned by the call.
  if (ret != SUCCESS)
  {
    return ret;
  }

  // copy the indexNum'th Index entry in buffer to memory ptr using memcpy

  /* the indexNum'th entry will begin at an offset of
     HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
  memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);

  // return SUCCESS
  return SUCCESS;
}

int IndLeaf::setEntry(void *ptr, int indexNum) {

    // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
    //     return E_OUTOFBOUND.
    if(indexNum<0||indexNum>MAX_KEYS_LEAF-1){
      return E_OUTOFBOUND;
    }

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
       int status=IndLeaf::loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
    if(status!=SUCCESS){
      return status;
    }

    // copy the Index at ptr to indexNum'th entry in the buffer using memcpy


    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    memcpy(entryPtr, (struct Index *)ptr, LEAF_ENTRY_SIZE);

    status=StaticBuffer::setDirtyBit(this->blockNum);
    
    if(status!=SUCCESS){
      return status;
    }

    return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum) {
    if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL) {
        return E_OUTOFBOUND;
    }

    unsigned char *bufferPtr;
      /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.

    // typecast the void pointer to an internal entry pointer

    int status = loadBlockAndGetBufferPtr(&bufferPtr);

    if (status != SUCCESS) {
        return status;
    }

    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

    memcpy(entryPtr, &(internalEntry->lChild), 4);
    memcpy(entryPtr + 4, &(internalEntry->attrVal), ATTR_SIZE);
    memcpy(entryPtr + 20, &(internalEntry->rChild), 4);

    status = StaticBuffer::setDirtyBit(this->blockNum);
    if (status != SUCCESS) {
        return status;
    }

    return SUCCESS;
}