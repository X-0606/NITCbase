#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[32][2048];
struct BufferMetaInfo StaticBuffer::metainfo[32];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
  for(int i = 0; i < 4; i++) {
    Disk::readBlock(blockAllocMap + i * BLOCK_SIZE, i);
  }

  for (int i = 0; i < 32; i++) {
    metainfo[i].free = true;
    metainfo[i].dirty = false;
    metainfo[i].blockNum = -1;
    metainfo[i].timeStamp = -1;
  }
}

int StaticBuffer::getStaticBlockType(int blockNum) {
  if(blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  return (int)blockAllocMap[blockNum];
}

StaticBuffer::~StaticBuffer() {
  for(int i = 0; i < 4; i++) {
    Disk::writeBlock(blockAllocMap + i * BLOCK_SIZE, i);
  }

  for(int i = 0; i < BUFFER_CAPACITY; i++) {
    if(metainfo[i].dirty == true && metainfo[i].free == false) {
        Disk::writeBlock(blocks[i], metainfo[i].blockNum);
    }
  }
}

int StaticBuffer::getFreeBuffer(int blockNum) {
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  for(int i = 0; i < BUFFER_CAPACITY; i++) {
    if(metainfo[i].free == false) {
      metainfo[i].timeStamp++;
    }
  }

  int allocatedBuffer = -79;

  for(int i = 0; i < 32; i++) {
    if(metainfo[i].free == true) {
        allocatedBuffer = i;
        break;
    }
  }
 
  if(allocatedBuffer == -79) {
    int maxtimestamp = -1;
    for(int i = 0; i < BUFFER_CAPACITY; i++) {
      if(maxtimestamp < metainfo[i].timeStamp) {
        maxtimestamp = metainfo[i].timeStamp;
        allocatedBuffer = i;
      }
    }
  }

  if(metainfo[allocatedBuffer].dirty == true) {
    Disk::writeBlock(blocks[allocatedBuffer], metainfo[allocatedBuffer].blockNum);
  }
  
  metainfo[allocatedBuffer].free = false;
  metainfo[allocatedBuffer].blockNum = blockNum;
  metainfo[allocatedBuffer].timeStamp = 0;
  metainfo[allocatedBuffer].dirty = false;

  return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum) {
  if(blockNum<0||blockNum>=DISK_BLOCKS){
    return E_OUTOFBOUND;
  }

  for(int i=0;i<32;i++){
    if(metainfo[i].blockNum==blockNum){
        return i;
    }
  }

  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){
  int buffernum=getBufferNum(blockNum);
  if(buffernum==E_OUTOFBOUND){
    return E_OUTOFBOUND;
  }

  if(buffernum==E_BLOCKNOTINBUFFER){
    return E_BLOCKNOTINBUFFER;
  }

  metainfo[buffernum].dirty=true;
  return SUCCESS;
}

