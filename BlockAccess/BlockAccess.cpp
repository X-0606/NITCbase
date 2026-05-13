#include "BlockAccess.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
    RecId prevRecId;
    int ret = RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relcatbuf;
        int relstat = RelCacheTable::getRelCatEntry(relId, &relcatbuf);
        block = relcatbuf.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while (block != -1)
    {
        RecBuffer obj(block);
        struct HeadInfo head;
        obj.getHeader(&head);
        unsigned char slotmap[head.numSlots];
        obj.getSlotMap(slotmap);

        if (slot >= head.numSlots)
        {
            block = head.rblock;
            slot = 0;
            continue;
        }

        if (slotmap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
        }

        Attribute record[head.numAttrs];
        obj.getRecord(record, slot);

        AttrCatEntry entry;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &entry);
        union Attribute attrVal2 = record[entry.offset];
        int type = entry.attrType;

        int cmpVal;
        cmpVal = compareAttrs(attrVal2, attrVal, type);

        if (
            (op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0) ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0) ||
            (op == GE && cmpVal >= 0))
        {
            prevRecId.block = block;
            prevRecId.slot = slot;
            RelCacheTable::setSearchIndex(relId, &prevRecId);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(0);
    char RelName[16] = "RelName";

    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);

    RecId id;
    id = linearSearch(0, RelName, newRelationName, EQ);

    if (id.block != -1 && id.slot != -1)
    {
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(0);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);
    id = linearSearch(0, RelName, oldRelationName, EQ);

    if (id.block == -1 && id.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    RecBuffer recblk(RELCAT_BLOCK);
    Attribute newrecord[RELCAT_NO_ATTRS];
    recblk.getRecord(newrecord, id.slot);
    strcpy(newrecord[RELCAT_REL_NAME_INDEX].sVal, newName);
    recblk.setRecord(newrecord, id.slot);

    RelCacheTable::resetSearchIndex(1);

    for (int i = 0; i < newrecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; i++)
    {
        id = linearSearch(1, RelName, oldRelationName, EQ);
        RecBuffer attrblk(id.block);
        Attribute newrecord_attr[ATTRCAT_NO_ATTRS];
        attrblk.getRecord(newrecord_attr, id.slot);
        strcpy(newrecord_attr[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        attrblk.setRecord(newrecord_attr, id.slot);
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(0);
    char RelName[16] = "RelName";

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId id;
    id = linearSearch(0, RelName, relNameAttr, EQ);
    if (id.block == -1 && id.slot == -1)
    {
        return E_RELNOTEXIST;
    }
    RelCacheTable::resetSearchIndex(1);

    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while (true)
    {
        id = linearSearch(1, RelName, relNameAttr, EQ);

        if (id.block == -1 && id.slot == -1)
        {
            break;
        }

        RecBuffer attrcatrec(id.block);
        attrcatrec.getRecord(attrCatEntryRecord, id.slot);

        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
            return E_ATTREXIST;

        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
        {
            attrToRenameRecId = id;
            break;
        }
    }
    if (attrToRenameRecId.slot == -1 && attrToRenameRecId.block == -1)
    {
        return E_ATTRNOTEXIST;
    }

    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord
    RecBuffer attrcatrec(attrToRenameRecId.block);
    attrcatrec.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    attrcatrec.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record)
{
    RelCatEntry relcatentry;
    RelCacheTable::getRelCatEntry(relId, &relcatentry);

    int blockNum = relcatentry.firstBlk;
    RecId rec_id = {-1, -1};
    int numOfSlots = relcatentry.numSlotsPerBlk;
    int numOfAttributes = relcatentry.numAttrs;
    int prevBlockNum = -1;

    while (blockNum != -1)
    {
        RecBuffer recblk(blockNum);
        HeadInfo head;
        recblk.getHeader(&head);
        unsigned char slotmap[head.numSlots];
        recblk.getSlotMap(slotmap);

        for (int i = 0; i < head.numSlots; i++)
        {
            if (slotmap[i] == SLOT_UNOCCUPIED)
            {
                rec_id.block = blockNum;
                rec_id.slot = i;
                break;
            }
        }
        if (rec_id.block != -1 && rec_id.slot != -1)
        {
            break;
        }

        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    if (rec_id.block == -1 && rec_id.slot == -1)
    {
        if (relId == 0)
        {
            return E_MAXRELATIONS;
        }

        RecBuffer newblk;
        int ret = newblk.getBlockNum();
        if (ret == E_DISKFULL)
        {
            return E_DISKFULL;
        }

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        rec_id.block = ret;
        rec_id.slot = 0;

        HeadInfo head;
        head.blockType = REC;
        head.rblock = -1;
        head.numEntries = 0;
        head.pblock = -1;
        head.lblock = prevBlockNum;
        head.numSlots = numOfSlots;
        head.numAttrs = numOfAttributes;
        newblk.setHeader(&head);

        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
        unsigned char slotmap[head.numSlots];
        for (int i = 0; i < head.numSlots; i++)
        {
            slotmap[i] = SLOT_UNOCCUPIED;
        }
        newblk.setSlotMap(slotmap);

        if (prevBlockNum != -1)
        {
            RecBuffer prevblk(prevBlockNum);
            HeadInfo prevhead;
            prevblk.getHeader(&prevhead);
            prevhead.rblock = rec_id.block;
            prevblk.setHeader(&prevhead);
        }
        else
        {
            relcatentry.firstBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relcatentry);
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
        relcatentry.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId, &relcatentry);
    }

    RecBuffer recblk(rec_id.block);
    recblk.setRecord(record, rec_id.slot);
    HeadInfo head;
    recblk.getHeader(&head);

    unsigned char slotmap[numOfSlots];
    recblk.getSlotMap(slotmap);
    slotmap[rec_id.slot] = SLOT_OCCUPIED;

    recblk.setSlotMap(slotmap);

    head.numEntries++;
    recblk.setHeader(&head);

    relcatentry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relcatentry);

    int flag = SUCCESS;

    for (int i = 0; i < relcatentry.numAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        int status = AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        if (status == SUCCESS && attrCatEntry.rootBlock != -1)
        {
            int retVal = BPlusTree::bPlusInsert(relId, attrCatEntry.attrName,
                                                record[i], rec_id);

            if (retVal == E_DISKFULL)
            {
                flag = E_INDEX_BLOCKS_RELEASED;
            }
        }
    }

    return flag;
}

int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    RecId recId;

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    if (ret != SUCCESS)
    {
        return ret;
    }

    int rootBlock = attrCatEntry.rootBlock;
    if (rootBlock == -1)
    {
        recId = linearSearch(relId, attrName, attrVal, op);
    }
    else
    {
        recId = BPlusTree::bPlusSearch(relId, attrName, attrVal, op);
    }

    if (recId.block == -1 && recId.slot == -1)
    {
        return E_NOTFOUND;
    }

    RecBuffer recordBuffer(recId.block);
    recordBuffer.getRecord(record, recId.slot);

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId relCatRecId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    if (relCatRecId.block == -1 && relCatRecId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer relCatBlock(relCatRecId.block);
    relCatBlock.getRecord(relCatEntryRecord, relCatRecId.slot);

    int currentBlockNum = (int)relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs = (int)relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    while (currentBlockNum != -1)
    {
        RecBuffer block(currentBlockNum);
        HeadInfo header;
        block.getHeader(&header);
        int nextBlockNum = header.rblock;
        block.releaseBlock();
        currentBlockNum = nextBlockNum;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    while (true)
    {
        RecId attrCatRecId;
        attrCatRecId = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
        {
            break;
        }

        numberOfAttributesDeleted++;

        RecBuffer attrCatBlock(attrCatRecId.block);

        HeadInfo attrCatHeader;
        attrCatBlock.getHeader(&attrCatHeader);

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);

        int rootBlock = (int)attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

        unsigned char slotMap[attrCatHeader.numSlots];
        attrCatBlock.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        attrCatBlock.setSlotMap(slotMap);

        attrCatHeader.numEntries--;
        attrCatBlock.setHeader(&attrCatHeader);

        if (attrCatHeader.numEntries == 0)
        {
            RecBuffer prevBlock(attrCatHeader.lblock);
            HeadInfo prevHeader;
            prevBlock.getHeader(&prevHeader);
            prevHeader.rblock = attrCatHeader.rblock;
            prevBlock.setHeader(&prevHeader);

            if (attrCatHeader.rblock != -1)
            {
                RecBuffer nextBlock(attrCatHeader.rblock);
                HeadInfo nextHeader;
                nextBlock.getHeader(&nextHeader);
                nextHeader.lblock = attrCatHeader.lblock;
                nextBlock.setHeader(&nextHeader);
            }
            else
            {
                RelCatEntry attrCatRelEntry;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
                attrCatRelEntry.lastBlk = attrCatHeader.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
            }
            attrCatBlock.releaseBlock();
        }

        if (rootBlock != -1)
        {
            BPlusTree::bPlusDestroy(rootBlock);
        }
    }

    RecBuffer relCatBlockFinal(relCatRecId.block);
    HeadInfo relCatHeader;
    relCatBlockFinal.getHeader(&relCatHeader);

    relCatHeader.numEntries--;
    relCatBlockFinal.setHeader(&relCatHeader);

    unsigned char relCatSlotMap[relCatHeader.numSlots];
    relCatBlockFinal.getSlotMap(relCatSlotMap);
    relCatSlotMap[relCatRecId.slot] = SLOT_UNOCCUPIED;
    relCatBlockFinal.setSlotMap(relCatSlotMap);

    RelCatEntry relCatRelEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatRelEntry);
    relCatRelEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatRelEntry);

    RelCatEntry attrCatRelEntry;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
    attrCatRelEntry.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);

    return SUCCESS;
}

int BlockAccess::project(int relId, Attribute *record)
{
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;

    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }
    while (block != -1)
    {
        RecBuffer recBuffer(block);

        HeadInfo header;
        recBuffer.getHeader(&header);

        unsigned char slotMap[header.numSlots];
        recBuffer.getSlotMap(slotMap);

        if (slot >= header.numSlots)
        {
            block = header.rblock;
            slot = 0;
        }
        else if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
        }
        else
        {
            break;
        }
    }

    if (block == -1)
    {
        return E_NOTFOUND;
    }

    RecId nextRecId{block, slot};

    RelCacheTable::setSearchIndex(relId, &nextRecId);

    RecBuffer recordBuffer(block);
    recordBuffer.getRecord(record, slot);

    return SUCCESS;
}