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

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i)
  {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free=true;
  }


  // /Setting up Relation Cache entries /
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(4);
  Attribute relCatRecord[6];

  for(int i=0;i<2;i++){
  struct RelCacheEntry relCacheEntry;
  relCatBlock.getRecord(relCatRecord,i);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = i;
  RelCacheTable::relCache[i] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));//heap allocated since stack allocated gets done and dusted after the function ends
  *(RelCacheTable::relCache[i]) = relCacheEntry;
  }


// Attribute Catalog
for(int i=0; i<2; i++) { // 0 for RELCAT, 1 for ATTRCAT
    AttrCacheEntry *attrHead = NULL;
    AttrCacheEntry *temp = NULL;
    
    RecBuffer attrblk(5); 
    HeadInfo head;
    attrblk.getHeader(&head);
    Attribute attrCatRecord[6];

    for(int j=0; j < head.numEntries; j++) {
        attrblk.getRecord(attrCatRecord, j);

        if (strcmp(attrCatRecord[0].sVal, RelCacheTable::relCache[i]->relCatEntry.relName) == 0) {
            
            AttrCacheEntry *newnode = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
            AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newnode->attrCatEntry);
            newnode->recId.block = 5;
            newnode->recId.slot = j;
            newnode->next = NULL;

            if(attrHead == NULL) {
                attrHead = newnode;
                temp = attrHead;
            } else {
                temp->next = newnode;
                temp = temp->next;
            }
        }
    }
    AttrCacheTable::attrCache[i] = attrHead;
}

 OpenRelTable::tableMetaInfo[0].free=false;
  strcpy(OpenRelTable::tableMetaInfo[0].relName, RELCAT_RELNAME);

  OpenRelTable::tableMetaInfo[1].free=false;
  strcpy(OpenRelTable::tableMetaInfo[1].relName, ATTRCAT_RELNAME);


}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  for(int i=0;i<MAX_OPEN;i++){
     if(tableMetaInfo[i].free==false){
     if(strcmp(OpenRelTable::tableMetaInfo[i].relName,relName)==0){
      return i;
     }}
  }
  
  return E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry() {

      // for(int i=0;i<12;i++){
      //    std::cout<<tableMetaInfo[i].free<<std::endl;
      // }

  /* traverse through the OpenRelTable::tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
    for(int i=2;i<12;i++){
      if(OpenRelTable::tableMetaInfo[i].free==true){
        // std::cout<<"yeah returning" << i<<std::endl;
        return i;
      }
    }

    return E_CACHEFULL;

  // if found return the relation id, else return E_CACHEFULL.
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {

  if(getRelId(relName)!=E_RELNOTOPEN){
    // (checked using OpenRelTable::getRelId())
    return getRelId(relName);
  }

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
     int slot=getFreeOpenRelTableEntry();

  if (slot==E_CACHEFULL){
    return E_CACHEFULL;
  }
  char attrName[ATTR_SIZE];
  strcpy(attrName, "RelName");
  // let relId be used to store the free slot.
  int relId=slot;

  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/


  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relcatRecId;
  Attribute a;
  strcpy(a.sVal,relName);
  RelCacheTable::relCache[0]->searchIndex.block=-1;
  RelCacheTable::relCache[0]->searchIndex.slot=-1;
  relcatRecId=BlockAccess::linearSearch(0,attrName,a,EQ);

  if (relcatRecId.block==-1&&relcatRecId.slot==-1) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
 
  Attribute relCatRecord[6];
  RecBuffer relCatBlock(relcatRecId.block);
  struct RelCacheEntry relCacheEntry;
  relCatBlock.getRecord(relCatRecord,relcatRecId.slot);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = relcatRecId.slot;
  RelCacheTable::relCache[relId] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));//heap allocated since stack allocated gets done and dusted after the function ends
  *(RelCacheTable::relCache[relId]) = relCacheEntry;
  strcpy(tableMetaInfo[relId].relName,relName);



  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry* listHead=NULL;
  AttrCacheEntry* temp=NULL;

  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/
  
   RelCacheTable::relCache[1]->searchIndex.block=-1;
   RelCacheTable::relCache[1]->searchIndex.slot=-1;


  for(int i=0;i<RelCacheTable::relCache[relId]->relCatEntry.numAttrs;i++)
  {
      /* let attrcatRecId store a valid record id an entry of the relation, relName,
      in the Attribute Catalog.*/
      RecId attrcatRecId;
      attrcatRecId=BlockAccess::linearSearch(1,attrName,a,EQ);
      RecBuffer attrcatblock(attrcatRecId.block);
      Attribute attrcatrecord[ATTRCAT_NO_ATTRS];
      attrcatblock.getRecord(attrcatrecord,attrcatRecId.slot);  
      AttrCacheEntry *newnode=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
      newnode->next=NULL;
      AttrCacheTable::recordToAttrCatEntry(attrcatrecord,&newnode->attrCatEntry);
      newnode->recId.block=attrcatRecId.block;
      newnode->recId.slot=attrcatRecId.slot;
      if(temp==NULL){
        listHead=newnode;
        temp=listHead;
      }
      else{
        temp->next=newnode;
        temp=temp->next;
      }
  }
    AttrCacheTable::attrCache[relId]=listHead;

  OpenRelTable::tableMetaInfo[relId].free=false;
  //  std::cout<<"yeah setted" << relId<<std::endl;
  strcpy(OpenRelTable::tableMetaInfo[relId].relName,relName);

  return relId;
}


int OpenRelTable::closeRel(int relId) {
  if (relId==1||relId==0) {
    return E_NOTPERMITTED;
  }

  if (relId>12||relId<0) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free==true) {
    return E_RELNOTOPEN;
  }

  
    if(OpenRelTable::tableMetaInfo[relId].free==false){
      if(RelCacheTable::relCache[relId]->dirty==true){
        Attribute relcat[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry),relcat);
        RecId id=RelCacheTable::relCache[relId]->recId;
        RecBuffer blk(id.block);
        blk.setRecord(relcat,id.slot);
      }
    }
  

  

  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function

  // update `tableMetaInfo` to set `relId` as a free slot
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
  free(RelCacheTable::relCache[relId]);
    AttrCacheEntry* temp=AttrCacheTable::attrCache[relId];
    while(temp!=NULL){
      AttrCacheEntry* curr=temp;
      temp=temp->next;
      free(curr);
    }
    tableMetaInfo[relId].free=true;


  return SUCCESS;
}



OpenRelTable::~OpenRelTable() {
    
    for (int i = 2; i < MAX_OPEN; ++i) {
        if (RelCacheTable::relCache[i] != nullptr) {
            OpenRelTable::closeRel(i);
        }
    }

    /**** Closing the catalog relations in the relation cache ****/

    // releasing the relation cache entry of the attribute catalog
    // if (/* RelCatEntry of the ATTRCAT_RELID-th RelCacheEntry has been modified */)
    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty == true) {

        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);

        Attribute relCatRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry, relCatRecord);

        RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;

         RecBuffer relCatBlock(recId.block);


        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
        relCatBlock.setRecord(relCatRecord, recId.slot);
    }
    // free the memory dynamically allocated to this RelCacheEntry
    free(RelCacheTable::relCache[ATTRCAT_RELID]);


    // releasing the relation cache entry of the relation catalog
    // if(/* RelCatEntry of the RELCAT_RELID-th RelCacheEntry has been modified */)
    if (RelCacheTable::relCache[RELCAT_RELID]->dirty == true) {

        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);

        Attribute relCatRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry, relCatRecord);

        RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;

        // declaring an object of RecBuffer class to write back to the buffer
        RecBuffer relCatBlock(recId.block);

        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
        relCatBlock.setRecord(relCatRecord, recId.slot);
    }
    // free the memory dynamically allocated for this RelCacheEntry
    free(RelCacheTable::relCache[RELCAT_RELID]);


    // free the memory allocated for the attribute cache entries of the
    // relation catalog and the attribute catalog
    for (int i = 0; i < 2; i++) {
        AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
        while (curr != nullptr) {
            AttrCacheEntry *next = curr->next;
            free(curr);
            curr = next;
        }
    }
}