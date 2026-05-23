#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "../Disk_Class/Disk.h"

int count = 0;

BlockBuffer::BlockBuffer(int blockNum)
{
  this->blockNum = blockNum;
}

BlockBuffer::BlockBuffer(char blockType)
{
  int blockTypeInt;
  if (blockType == 'R')
    blockTypeInt = REC;
  else if (blockType == 'I')
    blockTypeInt = IND_INTERNAL;
  else if (blockType == 'L')
    blockTypeInt = IND_LEAF;

  int allocatedblk = BlockBuffer::getFreeBlock(blockTypeInt);
  this->blockNum = allocatedblk;
}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

RecBuffer::RecBuffer() : BlockBuffer('R') {}

IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType) {}
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum) {}
IndInternal::IndInternal() : IndBuffer('I') {}
IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum) {}
IndLeaf::IndLeaf() : IndBuffer('L') {}
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum) {}

int BlockBuffer::getHeader(struct HeadInfo *head)
{
  unsigned char *buffer;

  int retu = loadBlockAndGetBufferPtr(&buffer);

  if (retu != SUCCESS)
  {
    return retu;
  }
  memcpy(&head->blockType, buffer + 0, 4);
  memcpy(&head->pblock, buffer + 4, 4);
  memcpy(&head->numSlots, buffer + 24, 4);
  memcpy(&head->numEntries, buffer + 16, 4);
  memcpy(&head->numAttrs, buffer + 20, 4);
  memcpy(&head->rblock, buffer + 12, 4);
  memcpy(&head->lblock, buffer + 8, 4);

  return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec, int slotNum)
{
  struct HeadInfo head;
  this->getHeader(&head);
  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  unsigned char *buffer;

  int retu = loadBlockAndGetBufferPtr(&buffer);

  if (retu != SUCCESS)
  {
    return retu;
  }

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = buffer + HEADER_SIZE + head.numSlots + slotNum * recordSize;

  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum != E_BLOCKNOTINBUFFER)
  {
    StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
      if (i != bufferNum)
      {
        if (StaticBuffer::metainfo[i].free == false)
        {
          StaticBuffer::metainfo[i].timeStamp++;
        }
      }
    }
  }

  if (bufferNum == E_BLOCKNOTINBUFFER)
  {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND)
    {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  *buffPtr = StaticBuffer::blocks[bufferNum];
  return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *slotMap)
{
  unsigned char *bufferPtr;

  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS)
  {
    return ret;
  }

  struct HeadInfo head;
  getHeader(&head);

  int slotCount = head.numSlots;
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
  memcpy(slotMap, slotMapInBuffer, slotCount);

  return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{
  double diff;
  if (attrType == STRING)
    diff = strcmp(attr1.sVal, attr2.sVal);
  else if (attrType == NUMBER)
  {
    diff = attr1.nVal - attr2.nVal;
  }

  count++;

  if (diff > 0)
  {
    return 1;
  }
  else if (diff < 0)
  {
    return -1;
  }
  else
  {
    return 0;
  }
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum)
{
  unsigned char *bufferPtr;
  int status = loadBlockAndGetBufferPtr(&bufferPtr);

  if (status != SUCCESS)
  {
    return status;
  }

  HeadInfo head;
  getHeader(&head);

  int noattrs = head.numAttrs;
  int slots = head.numSlots;

  if (slotNum >= slots)
  {
    return E_OUTOFBOUND;
  }

  memcpy(bufferPtr + HEADER_SIZE + head.numSlots + (slotNum * (ATTR_SIZE * noattrs)), rec, 16 * noattrs);

  StaticBuffer::setDirtyBit(this->blockNum);

  return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head)
{
  unsigned char *bufferPtr;
  int s = loadBlockAndGetBufferPtr(&bufferPtr);

  if (s != SUCCESS)
  {
    return s;
  }

  struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

  bufferHeader->lblock = head->lblock;
  bufferHeader->numAttrs = head->numAttrs;
  bufferHeader->numEntries = head->numEntries;
  bufferHeader->numSlots = head->numSlots;
  bufferHeader->pblock = head->pblock;
  bufferHeader->rblock = head->rblock;
  bufferHeader->blockType = head->blockType;

  s = StaticBuffer::setDirtyBit(this->blockNum);
  if (s != SUCCESS)
  {
    return s;
  }

  return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType)
{
  unsigned char *bufferPtr;
  int s = loadBlockAndGetBufferPtr(&bufferPtr);

  if (s != SUCCESS)
  {
    return s;
  }

  *((int32_t *)bufferPtr) = blockType;
  StaticBuffer::blockAllocMap[this->blockNum] = (unsigned char)blockType;

  s = StaticBuffer::setDirtyBit(this->blockNum);
  if (s != SUCCESS)
  {
    return s;
  }

  return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType)
{
  int blk = -1;
  for (int i = 0; i < 8192; i++)
  {
    if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK)
    {
      blk = i;
      break;
    }
  }

  if (blk == -1)
  {
    return E_DISKFULL;
  }

  this->blockNum = blk;
  int allocatedbuffernum = StaticBuffer::getFreeBuffer(blk);

  HeadInfo head;
  head.blockType = (int32_t)blockType;
  head.pblock = -1;
  head.lblock = -1;
  head.rblock = -1;
  head.numSlots = 0;
  head.numAttrs = 0;
  head.numEntries = 0;

  this->setHeader(&head);
  this->setBlockType(blockType);

  return blk;
}

int RecBuffer::setSlotMap(unsigned char *slotMap)
{
  unsigned char *bufferPtr;
  int s = loadBlockAndGetBufferPtr(&bufferPtr);

  if (s != SUCCESS)
  {
    return s;
  }

  HeadInfo head;
  getHeader(&head);

  int numSlots = head.numSlots;

  memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);

  s = StaticBuffer::setDirtyBit(this->blockNum);

  return SUCCESS;
}

void BlockBuffer::releaseBlock()
{
  if (this->blockNum == INVALID_BLOCKNUM)
  {
    return;
  }

  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum != E_BLOCKNOTINBUFFER)
  {
    StaticBuffer::metainfo[bufferNum].free = true;
  }

  StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;

  this->blockNum = INVALID_BLOCKNUM;
}

int BlockBuffer::getBlockNum()
{

  return this->blockNum;
}

int IndInternal::getEntry(void *ptr, int indexNum)
{
  if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL)
  {
    return E_OUTOFBOUND;
  }

  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  if (ret != SUCCESS)
  {
    return ret;
  }

  struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;
  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

  memcpy(&(internalEntry->lChild), entryPtr, sizeof(int32_t));
  memcpy(&(internalEntry->attrVal), entryPtr + 4, sizeof(Attribute));
  memcpy(&(internalEntry->rChild), entryPtr + 20, 4);

  return SUCCESS;
}
int IndLeaf::getEntry(void *ptr, int indexNum)
{

  if (indexNum < 0 || indexNum >= MAX_KEYS_LEAF)
  {
    return E_OUTOFBOUND;
  }

  unsigned char *bufferPtr;

  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  if (ret != SUCCESS)
  {
    return ret;
  }

  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
  memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);

  return SUCCESS;
}

int IndLeaf::setEntry(void *ptr, int indexNum)
{

  if (indexNum < 0 || indexNum > MAX_KEYS_LEAF - 1)
  {
    return E_OUTOFBOUND;
  }

  unsigned char *bufferPtr;

  int status = IndLeaf::loadBlockAndGetBufferPtr(&bufferPtr);

  if (status != SUCCESS)
  {
    return status;
  }

  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
  memcpy(entryPtr, (struct Index *)ptr, LEAF_ENTRY_SIZE);

  status = StaticBuffer::setDirtyBit(this->blockNum);

  if (status != SUCCESS)
  {
    return status;
  }

  return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum)
{
  if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL)
  {
    return E_OUTOFBOUND;
  }

  unsigned char *bufferPtr;
  int status = loadBlockAndGetBufferPtr(&bufferPtr);

  if (status != SUCCESS)
  {
    return status;
  }

  struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;
  unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

  memcpy(entryPtr, &(internalEntry->lChild), 4);
  memcpy(entryPtr + 4, &(internalEntry->attrVal), ATTR_SIZE);
  memcpy(entryPtr + 20, &(internalEntry->rChild), 4);

  status = StaticBuffer::setDirtyBit(this->blockNum);
  if (status != SUCCESS)
  {
    return status;
  }

  return SUCCESS;
}