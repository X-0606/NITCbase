#include "BlockAccess.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
     RecId prevRecId;
     int ret=RelCacheTable::getSearchIndex(relId,&prevRecId);

    // let block and slot denote the record id of the record being currently checked
    int block,slot;
    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)
        RelCatEntry relcatbuf;
        int relstat=RelCacheTable::getRelCatEntry(relId,&relcatbuf);
        
        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)

        // block = first record block of the relation
        block=relcatbuf.firstBlk;
        // slot = 0
        slot=0;
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)

        // block = search index's block
        block=prevRecId.block;
        // slot = search index's slot + 1
        slot=prevRecId.slot+1;
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        RecBuffer obj(block);
        // get the record with id (block, slot) using RecBuffer::getRecord()
         struct HeadInfo head;
        
        // get header of the block using RecBuffer::getHeader() function
        obj.getHeader(&head);
        unsigned char slotmap[head.numSlots];
        obj.getSlotMap(slotmap);
        // get slot map of the block using RecBuffer::getSlotMap() function
        
        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if(slot>=head.numSlots)
        {
            // update block = right block of block
            block=head.rblock;
            slot=0;
            // update slot = 0
            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if(slotmap[slot]==SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
            // increment slot and continue to the next record slot
        }

        Attribute record[head.numAttrs];
        obj.getRecord(record,slot);

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        /* use the attribute offset to get the value of the attribute from
           current record */
           AttrCatEntry entry;
           AttrCacheTable::getAttrCatEntry(relId,attrName,&entry);
           union Attribute attrVal2=record[entry.offset];
           int type=entry.attrType;


        int cmpVal;  // will store the difference between the attributes
        // set cmpVal using compareAttrs()
        cmpVal=compareAttrs(attrVal2,attrVal,type);

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            prevRecId.block=block;
            prevRecId.slot= slot;
            RelCacheTable::setSearchIndex(relId,&prevRecId);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
       RelCacheTable::resetSearchIndex(0);
       char RelName[16]="RelName";

    Attribute newRelationName; 
    strcpy(newRelationName.sVal,newName);   // set newRelationName with newName

    // search the relation catalog for an entry with RelName = newRelationName
    RecId id;
    id=linearSearch(0,RelName,newRelationName,EQ);

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    if(id.block!=-1&&id.slot!=-1){
       return E_RELEXIST;
    }

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
     RelCacheTable::resetSearchIndex(0);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal,oldName);  
      // set oldRelationName with oldName
    id=linearSearch(0,RelName,oldRelationName,EQ);

    // search the relation catalog for an entry with RelName = oldRelationName

    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
     if(id.block==-1&&id.slot==-1){
       return E_RELNOTEXIST;
    }
     
    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
   RecBuffer recblk(RELCAT_BLOCK);
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    Attribute newrecord[RELCAT_NO_ATTRS];
    recblk.getRecord(newrecord,id.slot);
    strcpy(newrecord[RELCAT_REL_NAME_INDEX].sVal,newName);
    // set back the record value using RecBuffer.setRecord
    recblk.setRecord(newrecord,id.slot);

    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */


    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
       RelCacheTable::resetSearchIndex(1);

    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord

    for(int i=0;i<newrecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;i++){
        id=linearSearch(1,RelName,oldRelationName,EQ);
        RecBuffer attrblk(id.block);
        Attribute newrecord_attr[ATTRCAT_NO_ATTRS];
        attrblk.getRecord(newrecord_attr,id.slot);
        strcpy(newrecord_attr[ATTRCAT_REL_NAME_INDEX].sVal,newName);
        attrblk.setRecord(newrecord_attr,id.slot);
    }

    return SUCCESS;
}


int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */

    RelCacheTable::resetSearchIndex(0);
    char RelName[16]="RelName";

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal,relName);

    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    RecId id;
    
    id=linearSearch(0,RelName,relNameAttr,EQ);
    if(id.block==-1&&id.slot==-1){
        return E_RELNOTEXIST;
    }
    RelCacheTable::resetSearchIndex(1);

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */


    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog forRelName= relNameAttr
        id=linearSearch(1,RelName,relNameAttr,EQ);


        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        if(id.block==-1&&id.slot==-1){
            break;
        }

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
          RecBuffer attrcatrec(id.block);
          attrcatrec.getRecord(attrCatEntryRecord,id.slot);

          if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)==0)
          return E_ATTREXIST;

          if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName)==0){
              attrToRenameRecId=id;
            //   std::cout<<"Gothca"<<std::endl;
              break;
          }
         
          

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
    }
    if(attrToRenameRecId.slot==-1&&attrToRenameRecId.block==-1){
        //  std::cout<<"nope"<<std::endl;
        return E_ATTRNOTEXIST;
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;



    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord
    RecBuffer attrcatrec(attrToRenameRecId.block);
    attrcatrec.getRecord(attrCatEntryRecord,attrToRenameRecId.slot);
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
    attrcatrec.setRecord(attrCatEntryRecord,attrToRenameRecId.slot);
    

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)

    RelCatEntry relcatentry;
    RelCacheTable::getRelCatEntry(relId,&relcatentry);

    int blockNum = relcatentry.firstBlk;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relcatentry.numSlotsPerBlk;
    int numOfAttributes = relcatentry.numAttrs;

    int prevBlockNum = -1;

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */
    while (blockNum != -1) {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        RecBuffer recblk(blockNum);

        // get header of block(blockNum) using RecBuffer::getHeader() function
        HeadInfo head;
        recblk.getHeader(&head);

        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        unsigned char slotmap[head.numSlots];
        recblk.getSlotMap(slotmap);

        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */
        for(int i=0;i<head.numSlots;i++){
            if(slotmap[i]==SLOT_UNOCCUPIED){
                rec_id.block=blockNum;
                rec_id.slot=i;
                break;
            }
        }
        if(rec_id.block!=-1&&rec_id.slot!=-1){
            break;
        }

        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */

        
              prevBlockNum = blockNum;
              blockNum = head.rblock;
        
    }

    if(rec_id.block==-1&&rec_id.slot==-1)
    {
         if(relId==0){
         return E_MAXRELATIONS;
         }
             

        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        RecBuffer newblk;
        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
        int ret=newblk.getBlockNum();
        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        rec_id.block=ret;
        rec_id.slot=0;

       HeadInfo head;
       head.blockType=REC;
       head.rblock=-1;
       head.numEntries=0;
       head.pblock=-1;
       head.lblock=prevBlockNum;
       head.numSlots=numOfSlots;
       head.numAttrs=numOfAttributes;
       newblk.setHeader(&head);

        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
       unsigned char slotmap[head.numSlots];
       for(int i=0;i<head.numSlots;i++){
        slotmap[i]=SLOT_UNOCCUPIED;
       }
       newblk.setSlotMap(slotmap);

        if (prevBlockNum != -1)
        {
            // create a RecBuffer object for prevBlockNum
            RecBuffer prevblk(prevBlockNum);
            // get the header of the block prevBlockNum and
            // update the rblock field of the header to the new block
            HeadInfo prevhead;
            prevblk.getHeader(&prevhead);
            // number i.e. rec_id.block
            prevhead.rblock=rec_id.block;
            // (use BlockBuffer::setHeader() function)
            prevblk.setHeader(&prevhead);
        }
        else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
            relcatentry.firstBlk=rec_id.block;
            // printf("we are here\n");
            RelCacheTable::setRelCatEntry(relId,&relcatentry);
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
        relcatentry.lastBlk=rec_id.block;
        RelCacheTable::setRelCatEntry(relId,&relcatentry);
    
    }

    // create a RecBuffer object for rec_id.block
    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    // printf("Working\n");
    RecBuffer recblk(rec_id.block);
    recblk.setRecord(record,rec_id.slot);
    HeadInfo head;
    recblk.getHeader(&head);

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
        unsigned char slotmap[numOfSlots];
        recblk.getSlotMap(slotmap);
        slotmap[rec_id.slot]=SLOT_OCCUPIED;

       recblk.setSlotMap(slotmap);
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)

    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    head.numEntries++;
    recblk.setHeader(&head);

    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    relcatentry.numRecs++;
    RelCacheTable::setRelCatEntry(relId,&relcatentry);

    /* B+ Tree Insertions */
    int flag = SUCCESS;

    // Iterate over all the attributes of the relation
    for (int i = 0; i < relcatentry.numAttrs; i++) 
    {
        // get the attribute catalog entry for the attribute from the attribute cache
        AttrCatEntry attrCatEntry;
        int status = AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        // if index exists for the attribute (i.e. rootBlock != -1)
        if (status == SUCCESS && attrCatEntry.rootBlock != -1) 
        {
            /* insert the new record into the attribute's bplus tree using
             BPlusTree::bPlusInsert()*/
            int retVal = BPlusTree::bPlusInsert(relId, attrCatEntry.attrName,
                                                record[i], rec_id);

            if (retVal == E_DISKFULL) {
                flag = E_INDEX_BLOCKS_RELEASED; // blocks are released
            }
        }
    }

    return flag;

    
}


int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // Declare a variable called recid to store the searched record
    RecId recId;

   AttrCatEntry attrCatEntry;
    int ret=AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    

    // if this call returns an error, return the appropriate error code
    if(ret!=SUCCESS)
    {
        return ret;
    }

    // get rootBlock from the attribute catalog entry
    int rootBlock=attrCatEntry.rootBlock;
    if (rootBlock==-1) 
    {

        /* search for the record id (recid) corresponding to the attribute with
           attribute name attrName, with value attrval and satisfying the
           condition op using linearSearch()
        */
        recId=linearSearch(relId,attrName,attrVal,op);
    }

    else 
    {
        // (index exists for the attribute)

        /* search for the record id (recid) correspoding to the attribute with
        attribute name attrName and with value attrval and satisfying the
        condition op using BPlusTree::bPlusSearch() */
        recId=BPlusTree::bPlusSearch(relId,attrName,attrVal,op);
    }
    // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;
    if (recId.block == -1 && recId.slot == -1) {
        return E_NOTFOUND;
    }

    /* Copy the record with record id (recId) to the record buffer (record)
        For this Instantiate a RecBuffer class object using recId and
        call the appropriate method to fetch the record
    */
    RecBuffer recordBuffer(recId.block);
    recordBuffer.getRecord(record, recId.slot);

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //      return E_NOTPERMITTED
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
    strcpy(relNameAttr.sVal, relName);

    //  linearSearch on the relation catalog for RelName = relNameAttr
    RecId relCatRecId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    // if the relation does not exist (linearSearch returned {-1, -1})
    if (relCatRecId.block == -1 && relCatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
    RecBuffer relCatBlock(relCatRecId.block);
    relCatBlock.getRecord(relCatEntryRecord, relCatRecId.slot);

    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
    int currentBlockNum = (int)relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */
    int numAttrs = (int)relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    /*
      Delete all the record blocks of the relation
    */
    while (currentBlockNum != -1) {
        RecBuffer block(currentBlockNum);
        HeadInfo header;
        block.getHeader(&header);
        int nextBlockNum = header.rblock;

        block.releaseBlock();
        currentBlockNum = nextBlockNum;
    }

    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/

    // reset the searchIndex of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    while (true) {
        RecId attrCatRecId;
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr
        attrCatRecId = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1) {
            break;
        }

        numberOfAttributesDeleted++;

        // create a RecBuffer for attrCatRecId.block
        RecBuffer attrCatBlock(attrCatRecId.block);
        
        // get the header of the block
        HeadInfo attrCatHeader;
        attrCatBlock.getHeader(&attrCatHeader);
        
        // get the record corresponding to attrCatRecId.slot
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);

        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
        int rootBlock = (int)attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
        unsigned char slotMap[attrCatHeader.numSlots];
        attrCatBlock.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        attrCatBlock.setSlotMap(slotMap);

        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header */
        attrCatHeader.numEntries--;
        attrCatBlock.setHeader(&attrCatHeader);

        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list. */
        if (attrCatHeader.numEntries == 0) {
            /* Get the header of the left block and set it's rblock to this
               block's rblock */
            RecBuffer prevBlock(attrCatHeader.lblock);
            HeadInfo prevHeader;
            prevBlock.getHeader(&prevHeader);
            prevHeader.rblock = attrCatHeader.rblock;
            prevBlock.setHeader(&prevHeader);

            if (attrCatHeader.rblock != -1) {
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
                RecBuffer nextBlock(attrCatHeader.rblock);
                HeadInfo nextHeader;
                nextBlock.getHeader(&nextHeader);
                nextHeader.lblock = attrCatHeader.lblock;
                nextBlock.setHeader(&nextHeader);
            } else {
                // (the block being released is the "Last Block" of the relation.)
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
                RelCatEntry attrCatRelEntry;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
                attrCatRelEntry.lastBlk = attrCatHeader.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
            }
            attrCatBlock.releaseBlock();
        }

        // if index exists for the attribute (rootBlock != -1), call bplus destroy
        if (rootBlock != -1) {
            BPlusTree::bPlusDestroy(rootBlock);
        }
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    RecBuffer relCatBlockFinal(relCatRecId.block);
    HeadInfo relCatHeader;
    relCatBlockFinal.getHeader(&relCatHeader);

    /* Decrement the numEntries in the header */
    relCatHeader.numEntries--;
    relCatBlockFinal.setHeader(&relCatHeader);

    /* Update slotmap */
    unsigned char relCatSlotMap[relCatHeader.numSlots];
    relCatBlockFinal.getSlotMap(relCatSlotMap);
    relCatSlotMap[relCatRecId.slot] = SLOT_UNOCCUPIED;
    relCatBlockFinal.setSlotMap(relCatSlotMap);

    /*** Updating the Relation Cache Table ***/
    
    // Update Relation Catalog count
    RelCatEntry relCatRelEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatRelEntry);
    relCatRelEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatRelEntry);

    // Update Attribute Catalog count
    RelCatEntry attrCatRelEntry;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
    attrCatRelEntry.numRecs-= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);

    return SUCCESS;
}


int BlockAccess::project(int relId, Attribute *record) {
    // get the previous search index of the relation relId from the relation
    // cache (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    // declare block and slot which will be used to store the record id of the
    // slot we need to check.
    int block, slot;

    /* if the current search index record is invalid(i.e. = {-1, -1})
       (this only happens when the caller reset the search index)
    */
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (new project operation. start from beginning)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        // block = first record block of the relation
        block = relCatEntry.firstBlk;
        // slot = 0
        slot = 0;
    }
    else
    {
        // (a project/search operation is already in progress)

        // block = previous search index's block
        block = prevRecId.block;
        // slot = previous search index's slot + 1
        slot = prevRecId.slot + 1;
    }


    // The following code finds the next record of the relation
    /* Start from the record id (block, slot) and iterate over the remaining
       records of the relation */
    while (block != -1)
    {
        // create a RecBuffer object for block (using appropriate constructor!)
        RecBuffer recBuffer(block);

        // get header of the block using RecBuffer::getHeader() function
        HeadInfo header;
        recBuffer.getHeader(&header);
        
        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char slotMap[header.numSlots];
        recBuffer.getSlotMap(slotMap);

        if(slot >= header.numSlots)
        {
            // (no more slots in this block)
            // update block = right block of block
            block = header.rblock;
            // update slot = 0
            slot = 0;
            // (NOTE: if this is the last block, rblock would be -1. this would
            //        set block = -1 and fail the loop condition )
        }
        else if (slotMap[slot] == SLOT_UNOCCUPIED)
        { // (i.e slot-th entry in slotMap contains SLOT_UNOCCUPIED)

            // increment slot
            slot++;
        }
        else {
            // (the next occupied slot / record has been found)
            break;
        }
    }

    if (block == -1){
        // (a record was not found. all records exhausted)
        return E_NOTFOUND;
    }

    // declare nextRecId to store the RecId of the record found
    RecId nextRecId{block, slot};

    // set the search index to nextRecId using RelCacheTable::setSearchIndex
    RelCacheTable::setSearchIndex(relId, &nextRecId);

    /* Copy the record with record id (nextRecId) to the record buffer (record)
       For this Instantiate a RecBuffer class object by passing the recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recordBuffer(block);
    recordBuffer.getRecord(record, slot);

    return SUCCESS;
}