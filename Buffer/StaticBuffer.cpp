#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"

unsigned char StaticBuffer::blocks[32][2048];
struct BufferMetaInfo StaticBuffer::metainfo[32];

unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {

  for(int i=0;i<4;i++){
   Disk::readBlock(blockAllocMap+i*BLOCK_SIZE,i);
  }

  

  // initialise all blocks as free
  for (int i=0;i<32;i++) {
    metainfo[i].free = true;
    metainfo[i].dirty=false;
    metainfo[i].blockNum=-1;
    metainfo[i].timeStamp=-1;
  }

}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {
   
   for(int i=0;i<4;i++){
   Disk::writeBlock(blockAllocMap+i*BLOCK_SIZE,i);
  }

   for(int i=0;i<BUFFER_CAPACITY;i++){
    if(metainfo[i].dirty==true && metainfo[i].blockNum != -1){
        Disk::writeBlock(blocks[i],metainfo[i].blockNum);
    }
   }


}

int StaticBuffer::getFreeBuffer(int blockNum) {
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  for(int i=0;i<BUFFER_CAPACITY;i++){
    if(metainfo[i].free==false){
      metainfo[i].timeStamp++;
    }
  }

  int allocatedBuffer=-79;

  // iterate through all the blocks in the StaticBuffer
  // find the first free block in the buffer (check metainfo)
  // assign allocatedBuffer = index of the free block

 for(int i=0;i<32;i++){
    if(metainfo[i].free==true){
        allocatedBuffer=i;
        break;
    }
 }
 
 if(allocatedBuffer==-79){
  int maxtimestamp=-1;
    for(int i=0;i<BUFFER_CAPACITY;i++){
      if(maxtimestamp<metainfo[i].timeStamp){
        maxtimestamp=metainfo[i].timeStamp;
        allocatedBuffer=i;
      }
    }
 }
     if(metainfo[allocatedBuffer].dirty==true){
      Disk::writeBlock(blocks[allocatedBuffer],metainfo[allocatedBuffer].blockNum);
     }
      metainfo[allocatedBuffer].free = false;
       metainfo[allocatedBuffer].blockNum = blockNum;
       metainfo[allocatedBuffer].timeStamp=0;
       metainfo[allocatedBuffer].dirty=false;
       return allocatedBuffer;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.

  if(blockNum<0||blockNum>=DISK_BLOCKS){
    return E_OUTOFBOUND;
  }

  for(int i=0;i<32;i++){
    if(metainfo[i].blockNum==blockNum){
        return i;
    }
  }


  // find and return the bufferIndex which corresponds to blockNum (check metainfo)

  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){
    // find the buffer index corresponding to the block using getBufferNum().
    int buffernum=getBufferNum(blockNum);
     if(buffernum==E_OUTOFBOUND){
      return E_OUTOFBOUND;
    }

    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
    if(buffernum==E_BLOCKNOTINBUFFER){
      return E_BLOCKNOTINBUFFER;
    }
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND
   else{
    metainfo[buffernum].dirty=true;
   }
    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo

    // return SUCCESS
    return SUCCESS;
}