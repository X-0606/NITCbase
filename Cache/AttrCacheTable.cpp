#include "AttrCacheTable.h"
#include "../Buffer/BlockBuffer.h"
#include "../Cache/RelCacheTable.h" 
#include "../Cache/AttrCacheTable.h" 
#include <cstring>
#include <iostream>


// typedef struct AttrCatEntry {
//     unsigned char relName[ATTR_SIZE];
//     unsigned char attrName[ATTR_SIZE];
//     int attrType;
//     bool primaryFlag;
//     int rootBlock;
//     int offset;

// } AttrCatEntry;

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];

/* returns the attrOffset-th attribute for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuf) {
  // check if 0 <= relId < MAX_OPEN and return E_OUTOFBOUND otherwise
  if(relId<0||relId>12){
    return E_OUTOFBOUND;
  }

  if(attrCache[relId]==nullptr){
    return E_RELNOTOPEN;
  }

  // check if attrCache[relId] == nullptr and return E_RELNOTOPEN if true

  // traverse the linked list of attribute cache entries
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset == attrOffset) {
      *attrCatBuf=entry->attrCatEntry;
      return SUCCESS;
      // copy entry->attrCatEntry to *attrCatBuf and return SUCCESS;
    }
  }

  // there is no attribute at this offset
  return E_ATTRNOTEXIST;
}

/* Converts a attribute catalog record to AttrCatEntry struct
    We get the record as Attribute[] from the BlockBuffer.getRecord() function.
    This function will convert that to a struct AttrCatEntry type.
*/
void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS],
                                          AttrCatEntry* attrCatEntry) {
  strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
  strcpy(attrCatEntry->attrName, record[1].sVal);
  attrCatEntry->attrType=record[2].nVal;
  attrCatEntry->primaryFlag=(int)record[3].nVal;
  attrCatEntry->rootBlock=(int)record[4].nVal;
  attrCatEntry->offset=(int)record[5].nVal;
  


  // copy the rest of the fields in the record to the attrCacheEntry struct
}

int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatBuf) {

  // check that relId is valid and corresponds to an open relation


  // iterate over the entries in the attribute cache and set attrCatBuf to the entry that
  //    matches attrName
  if(relId<0||relId>12){
    return E_OUTOFBOUND;
  }

  if(attrCache[relId]==nullptr){
    return E_RELNOTOPEN;
  }

  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (strcmp(entry->attrCatEntry.attrName,attrName) == 0) {
      *attrCatBuf=entry->attrCatEntry;
      return SUCCESS;
      // copy entry->attrCatEntry to *attrCatBuf and return SUCCESS;
    }
  }

  // no attribute with name attrName for the relation
  return E_ATTRNOTEXIST;
}


int AttrCacheTable::getSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) 
{

  if(relId<0 || relId>=MAX_OPEN) 
  {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId]==nullptr) 
  {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry* entry=attrCache[relId];entry!=nullptr;entry=entry->next) 
  {
    if(strcmp(entry->attrCatEntry.attrName,attrName)==0) 
    {
      *searchIndex=entry->searchIndex;
      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}
int AttrCacheTable::getSearchIndex(int relId,int attrOffset, IndexId *searchIndex) 
{

  if(relId<0 || relId>=MAX_OPEN) 
  {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId]==nullptr) 
  {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry* entry=attrCache[relId];entry!=nullptr;entry=entry->next) 
  {
    if(entry->attrCatEntry.offset==attrOffset) 
    {
      *searchIndex=entry->searchIndex;
      return SUCCESS;
    }
  }
  

  return E_ATTRNOTEXIST;
}
int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) 
{

  if(relId<0 || relId>=MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId]==nullptr) 
  {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry* entry=attrCache[relId];entry!=nullptr;entry=entry->next) 
  {
    if(strcmp(entry->attrCatEntry.attrName,attrName)==0) 
    {
      entry->searchIndex=*searchIndex;
      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}
int AttrCacheTable::setSearchIndex(int relId,int attrOffset, IndexId *searchIndex) {

  if(relId<0 || relId>=MAX_OPEN) 
  {
    return E_OUTOFBOUND;
  }

  if(attrCache[relId]==nullptr) {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry* entry=attrCache[relId];entry!=nullptr;entry=entry->next) 
  {
    if(entry->attrCatEntry.offset==attrOffset) 
    {
      entry->searchIndex=*searchIndex;
      return SUCCESS;
    }
  }


  return E_ATTRNOTEXIST;
}
int AttrCacheTable::resetSearchIndex(int relId, char attrName[ATTR_SIZE])
{

  // declare an IndexId having value {-1, -1}
  // set the search index to {-1, -1} using AttrCacheTable::setSearchIndex
  // return the value returned by setSearchIndex
  IndexId searchIndex={-1,-1};
  int ret=setSearchIndex(relId,attrName,&searchIndex);
  return ret;
}
int AttrCacheTable::resetSearchIndex(int relId, int attrOffset) 
{

  // declare an IndexId having value {-1, -1}
  // set the search index to {-1, -1} using AttrCacheTable::setSearchIndex
  // return the value returned by setSearchIndex
   IndexId searchIndex={-1,-1};
  int ret=setSearchIndex(relId,attrOffset,&searchIndex);
  return ret;
}

int AttrCacheTable::setAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry *attrCatBuf) {

  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for(AttrCacheEntry *entry=attrCache[relId];entry!=nullptr;entry=entry->next)
  {
    if(strcmp(attrName,entry->attrCatEntry.attrName)==0)
    {
     entry->attrCatEntry = *attrCatBuf;
     entry->dirty = true;

      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId, int attrOffset, AttrCatEntry *attrCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset == attrOffset) {
      entry->attrCatEntry = *attrCatBuf;

      entry->dirty = true;

      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}

void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry *attrCatEntry, union Attribute record[ATTRCAT_NO_ATTRS]){
   
  strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal,attrCatEntry->attrName);
  strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal,attrCatEntry->attrName);
  record[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrCatEntry->attrType;
  record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = attrCatEntry->primaryFlag;
  record[ATTRCAT_ROOT_BLOCK_INDEX].nVal = attrCatEntry->rootBlock;
  record[ATTRCAT_OFFSET_INDEX].nVal = attrCatEntry->offset;

}