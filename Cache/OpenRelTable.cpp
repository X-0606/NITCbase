#include "OpenRelTable.h"
#include "../Buffer/BlockBuffer.h"
#include "../Cache/RelCacheTable.h"
#include "../Cache/AttrCacheTable.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable()
{
  for (int i = 0; i < MAX_OPEN; ++i)
  {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free = true;
  }

  // Setup Relation Cache entries for RELCAT and ATTRCAT
  RecBuffer relCatBlock(4);
  Attribute relCatRecord[6];

  for (int i = 0; i < 2; i++)
  {
    struct RelCacheEntry relCacheEntry;
    relCatBlock.getRecord(relCatRecord, i);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = i;
    RelCacheTable::relCache[i] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[i]) = relCacheEntry;
  }

  // Setup Attribute Cache entries
  for (int i = 0; i < 2; i++)
  {
    AttrCacheEntry *attrHead = NULL;
    AttrCacheEntry *temp = NULL;

    RecBuffer attrblk(5);
    HeadInfo head;
    attrblk.getHeader(&head);
    Attribute attrCatRecord[6];

    for (int j = 0; j < head.numEntries; j++)
    {
      attrblk.getRecord(attrCatRecord, j);

      if (strcmp(attrCatRecord[0].sVal, RelCacheTable::relCache[i]->relCatEntry.relName) == 0)
      {
        AttrCacheEntry *newnode = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newnode->attrCatEntry);
        newnode->recId.block = 5;
        newnode->recId.slot = j;
        newnode->next = NULL;

        if (attrHead == NULL)
        {
          attrHead = newnode;
          temp = attrHead;
        }
        else
        {
          temp->next = newnode;
          temp = temp->next;
        }
      }
    }
    AttrCacheTable::attrCache[i] = attrHead;
  }

  OpenRelTable::tableMetaInfo[0].free = false;
  strcpy(OpenRelTable::tableMetaInfo[0].relName, RELCAT_RELNAME);

  OpenRelTable::tableMetaInfo[1].free = false;
  strcpy(OpenRelTable::tableMetaInfo[1].relName, ATTRCAT_RELNAME);
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{
  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (tableMetaInfo[i].free == false)
    {
      if (strcmp(OpenRelTable::tableMetaInfo[i].relName, relName) == 0)
      {
        return i;
      }
    }
  }
  return E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry()
{
  for (int i = 2; i < 12; i++)
  {
    if (OpenRelTable::tableMetaInfo[i].free == true)
    {
      return i;
    }
  }
  return E_CACHEFULL;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
  if (getRelId(relName) != E_RELNOTOPEN)
  {
    return getRelId(relName);
  }

  int slot = getFreeOpenRelTableEntry();
  if (slot == E_CACHEFULL)
  {
    return E_CACHEFULL;
  }

  char attrName[ATTR_SIZE];
  strcpy(attrName, "RelName");
  int relId = slot;

  // Setup Relation Cache entry
  RecId relcatRecId;
  Attribute a;
  strcpy(a.sVal, relName);
  RelCacheTable::relCache[0]->searchIndex.block = -1;
  RelCacheTable::relCache[0]->searchIndex.slot = -1;
  relcatRecId = BlockAccess::linearSearch(0, attrName, a, EQ);

  if (relcatRecId.block == -1 && relcatRecId.slot == -1)
  {
    return E_RELNOTEXIST;
  }

  Attribute relCatRecord[6];
  RecBuffer relCatBlock(relcatRecId.block);
  struct RelCacheEntry relCacheEntry;
  relCatBlock.getRecord(relCatRecord, relcatRecId.slot);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = relcatRecId.slot;
  RelCacheTable::relCache[relId] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[relId]) = relCacheEntry;
  strcpy(tableMetaInfo[relId].relName, relName);

  // Setup Attribute Cache entry
  AttrCacheEntry *listHead = NULL;
  AttrCacheEntry *temp = NULL;

  RelCacheTable::relCache[1]->searchIndex.block = -1;
  RelCacheTable::relCache[1]->searchIndex.slot = -1;

  for (int i = 0; i < RelCacheTable::relCache[relId]->relCatEntry.numAttrs; i++)
  {
    RecId attrcatRecId;
    attrcatRecId = BlockAccess::linearSearch(1, attrName, a, EQ);
    RecBuffer attrcatblock(attrcatRecId.block);
    Attribute attrcatrecord[ATTRCAT_NO_ATTRS];
    attrcatblock.getRecord(attrcatrecord, attrcatRecId.slot);
    AttrCacheEntry *newnode = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
    newnode->next = NULL;
    AttrCacheTable::recordToAttrCatEntry(attrcatrecord, &newnode->attrCatEntry);
    newnode->recId.block = attrcatRecId.block;
    newnode->recId.slot = attrcatRecId.slot;
    if (temp == NULL)
    {
      listHead = newnode;
      temp = listHead;
    }
    else
    {
      temp->next = newnode;
      temp = temp->next;
    }
  }
  AttrCacheTable::attrCache[relId] = listHead;

  OpenRelTable::tableMetaInfo[relId].free = false;
  strcpy(OpenRelTable::tableMetaInfo[relId].relName, relName);

  return relId;
}

int OpenRelTable::closeRel(int relId)
{
  if (relId == 1 || relId == 0)
  {
    return E_NOTPERMITTED;
  }

  if (relId > 12 || relId < 0)
  {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free == true)
  {
    return E_RELNOTOPEN;
  }

  if (OpenRelTable::tableMetaInfo[relId].free == false)
  {
    if (RelCacheTable::relCache[relId]->dirty == true)
    {
      Attribute relcat[RELCAT_NO_ATTRS];
      RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry), relcat);
      RecId id = RelCacheTable::relCache[relId]->recId;
      RecBuffer blk(id.block);
      blk.setRecord(relcat, id.slot);
    }

    for (AttrCacheEntry *entry = AttrCacheTable::attrCache[relId]; entry != nullptr; entry = entry->next)
    {
      if (entry->dirty == true)
      {
        Attribute attrcat[ATTRCAT_NO_ATTRS];
        AttrCacheTable::attrCatEntryToRecord(&(entry->attrCatEntry), attrcat);
        RecId id = entry->recId;
        RecBuffer blk(id.block);
        blk.setRecord(attrcat, id.slot);
      }
    }
  }

  free(RelCacheTable::relCache[relId]);
  AttrCacheEntry *temp = AttrCacheTable::attrCache[relId];
  while (temp != NULL)
  {
    AttrCacheEntry *curr = temp;
    temp = temp->next;
    free(curr);
  }
  tableMetaInfo[relId].free = true;

  return SUCCESS;
}

OpenRelTable::~OpenRelTable()
{
  for (int i = 2; i < MAX_OPEN; ++i)
  {
    if (RelCacheTable::relCache[i] != nullptr)
    {
      OpenRelTable::closeRel(i);
    }
  }

  if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty == true)
  {
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCatEntry, relCatRecord);
    RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;
    RecBuffer relCatBlock(recId.block);
    relCatBlock.setRecord(relCatRecord, recId.slot);
  }
  free(RelCacheTable::relCache[ATTRCAT_RELID]);

  if (RelCacheTable::relCache[RELCAT_RELID]->dirty == true)
  {
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCatEntry, relCatRecord);
    RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;
    RecBuffer relCatBlock(recId.block);
    relCatBlock.setRecord(relCatRecord, recId.slot);
  }
  free(RelCacheTable::relCache[RELCAT_RELID]);

  for (int i = 0; i < 2; i++)
  {
    AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
    while (curr != nullptr)
    {
      AttrCacheEntry *next = curr->next;
      free(curr);
      curr = next;
    }
  }
}